
#include "ModelTest.hpp"
#include "ModelDefinitions.hpp"
#include "../ICScoreCalculator.hpp"
#include "corax/tree/treeinfo.h"

ModelTest::ModelTest(const Options &options, PartitionedMSA &msa, const Tree &tree, const IDVector &tip_msa_idmap,
                     const PartitionAssignment &part_assign,
                     Optimizer &optimizer): options(options), msa(msa), tree(tree), tip_msa_idmap(tip_msa_idmap),
                                            part_assign(part_assign), optimizer(optimizer) {
}


vector<std::string> generate_candidate_model_names(const DataType &dt) {
    vector<std::string> candidate_model_names;
    check_supported_datatype(dt);


    const auto &matrix_names = dt == DataType::dna ? dna_substitution_matrix_names : aa_model_names;

    for (const auto &subst_model: matrix_names) {
        for (const auto &frequency_type: default_frequency_type) {
            for (const auto &rate_heterogeneity: default_rate_heterogeneity) {
                const string model_descriptor = subst_model
                                                + frequency_type_label(dt, frequency_type)
                                                + rate_heterogeneity_label.at(rate_heterogeneity);

                const auto normalized_model_descriptor = normalize_model_name(model_descriptor);

                candidate_model_names.push_back(normalized_model_descriptor);
            }
        }
    }

    return candidate_model_names;
}

void ModelTest::optimize_model() {
    EvaluationResults results;

    vector<Model *> models;

    Options modified_options(options);
    modified_options.brlen_linkage = CORAX_BRLEN_UNLINKED;

    LOG_INFO << std::setprecision(20) << std::setfill(' ') << std::left;

    auto candidate_models = generate_candidate_model_names(msa.part_info(0).model().data_type());
    auto index = 1U;
    for (const auto &model_descriptor: candidate_models) {
        LOG_DEBUG << "[" << index << "/" << candidate_models.size() << "] Instantiating model " <<
                model_descriptor << endl;

        if (ParallelContext::thread_id() == 0) {
            // Only first thread updates models to prevent race-condition
            for (auto p = 0U; p < msa.part_count(); ++p) {
                models.push_back(new Model(model_descriptor));
                msa.model(p, *models.back());
            }
        }

        ParallelContext::thread_barrier();

        TreeInfo treeinfo(modified_options, tree, msa, tip_msa_idmap, part_assign);

        const double final_loglh = optimizer.optimize_model(treeinfo);

        LOG_INFO << "[" << index << "/" << candidate_models.size() << "]\tModel " << std::setw(12) <<
                model_descriptor << "\t\t" << final_loglh << endl;


        vector<PartitionModelEvaluation> partition_results(msa.part_count());

        for (auto p = 0U; p < msa.part_count(); ++p) {
            PartitionModelEvaluation partition_results;

            const double partition_loglh = treeinfo.pll_treeinfo().partition_loglh[p];
            const size_t free_params = msa.model(p).num_free_params() + treeinfo.tree().num_branches();
            const size_t sample_size = msa.part_info(p).stats().site_count;
            ICScoreCalculator ic_score_calculator(free_params, sample_size);

            partition_results.ic_criteria = ic_score_calculator.all(partition_loglh);
            partition_results.model = msa.model(p);
            partition_results.partition_loglh = partition_loglh;

            LOG_DEBUG << "\tPartition #" << p << " BIC " << partition_results.ic_criteria[
                InformationCriterion::bic] << endl;

            results.push_back(partition_results);
        }


        if (ParallelContext::thread_id() == 0) {
            for (const auto *m: models) {
                delete m;
            }
            models.clear();
        }
        ++index;
    }


    LOG_INFO << endl;
    for (auto p = 0U; p < msa.part_count(); ++p) {
        auto bic_ranking = rank_by_score(results, InformationCriterion::bic, p, msa.part_count());
        const auto &best_model = results[bic_ranking.at(0) * msa.part_count() + p];
        LOG_INFO << "Partition #" << p << ": " << best_model.model.to_string() << " BIC = " << best_model.
                ic_criteria.
                at(InformationCriterion::bic) << " LogLH = " << best_model.partition_loglh << endl;
    }


    if (ParallelContext::master()) {
        auto part_fname = options.output_fname("modeltest.partitions.bic.txt");
        fstream part_stream(part_fname, std::ios::out);
        for (auto p = 0U; p < msa.part_count(); ++p) {
            const auto &ranking = rank_by_score(results, InformationCriterion::bic, p, msa.part_count());
            const auto &best_model = results[ranking.at(0) * msa.part_count() + p];
            part_stream << best_model.model.to_string() << ", " << best_model.model.name() << " = " << msa.
                    part_info(p).
                    range_string() << endl;
        }
        part_stream.close();

        LOG_INFO << endl
                << "Partitions file (BIC) written to " << part_fname << endl;


        auto xml_fname = options.output_fname("modeltest.xml");
        fstream xml_stream(xml_fname, std::ios::out);
        print_xml(xml_stream, results, msa.part_count());
        xml_stream.close();

        LOG_INFO << "XML model selection file written to " << xml_fname << endl;
    }
}

vector<size_t> ModelTest::rank_by_score(const EvaluationResults &results,
                                        InformationCriterion ic,
                                        unsigned int partition_idx,
                                        unsigned int part_count) {
    std::vector<size_t> ranking(results.size() / part_count, 0);
    std::iota(ranking.begin(), ranking.end(), 0);

    std::sort(ranking.begin(), ranking.end(),
              [partition_idx, part_count, ic, &results](const size_t &a, const size_t &b) {
                  return results.at(a * part_count + partition_idx).ic_criteria.at(ic) < results.at(
                             b * part_count + partition_idx).
                         ic_criteria.at(ic);
              });

    return ranking;
}


vector<double> transform_delta_to_weight(const vector<double> &deltas) {
    vector<double> weights(deltas.size());
    weights.clear();

    for (const double &delta: deltas) {
        weights.push_back(std::exp(-delta / 2));
    }

    // normalize
    const double fac = 1 / std::accumulate(weights.begin(), weights.end(), 0.0);

    for (double &w: weights) {
        w *= fac;
    }

    return weights;
}


void ModelTest::print_xml(ostream &os, EvaluationResults &results, unsigned int partition_count) {
    os << setprecision(17);
    os << "<modeltestresults>" << endl;

    const auto ic_count = results.empty() ? 0 : results.at(0).ic_criteria.size();


    vector<double> deltas;
    deltas.reserve(results.size());

    for (auto p = 0U; p < partition_count; ++p) {
        os << "<partition id=\"" << p << "\">" << endl;

        for (auto ic_idx = 0U; ic_idx < ic_count; ++ic_idx) {
            const auto ic = static_cast<InformationCriterion>(ic_idx);

            const auto &ranking = rank_by_score(results, ic, p, partition_count);

            deltas.clear();

            const auto &best_score = results.at(ranking.at(0) * partition_count + p).ic_criteria.at(ic);
            for (auto i = 0U; i < results.size() / partition_count; ++i) {
                deltas.emplace_back(results[i * partition_count + p].ic_criteria.at(ic) - best_score);
            }

            const auto weights = transform_delta_to_weight(deltas);

            os << "<selection type=\"";

            switch (ic) {
                case InformationCriterion::aic:
                    os << "AIC";
                    break;
                case InformationCriterion::aicc:
                    os << "AICc";
                    break;
                case InformationCriterion::bic:
                    os << "BIC";
                    break;
            }

            os << "\">" << endl;

            for (auto i = 0U; i < results.size() / partition_count; ++i) {
                const auto &result = results[ranking.at(i) * partition_count + p];
                os << "<model rank=\"" << i + 1 << "\" name=\"" << result.model.to_string()
                        << "\" lnL=\"" << result.partition_loglh
                        << "\" score=\"" << result.ic_criteria.at(ic)
                        << "\" delta=\"" << deltas.at(ranking.at(i))
                        << "\" weight=\"" << weights.at(ranking.at(i))
                        << "\" />" << endl;
            }


            os << "</selection>" << endl;
        }

        os << "</partition>" << endl;
    }


    os << "</modeltestresults>" << endl;
}
