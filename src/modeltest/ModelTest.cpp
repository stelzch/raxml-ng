
#include "ModelTest.hpp"
#include "ModelDefinitions.hpp"
#include "../ICScoreCalculator.hpp"
#include "corax/tree/treeinfo.h"

ModelTest::ModelTest(const Options &options, PartitionedMSA &msa, const Tree &tree, const IDVector &tip_msa_idmap,
                     const PartitionAssignment &part_assign,
                     Optimizer &optimizer): options(options), msa(msa), tree(tree), tip_msa_idmap(tip_msa_idmap),
                                            part_assign(part_assign), optimizer(optimizer) {
}

void ModelTest::optimize_model() {
    const auto num_models = dna_substitution_matrix_names.size() * default_frequency_type.size() *
                            default_rate_heterogeneity.size();

    auto index = 1U;


    EvaluationResults results;

    for (const auto &entry: dna_substitution_matrix_names) {
        const auto subst_model = entry.first;

        LOG_INFO << std::setprecision(20) << std::setfill(' ') << std::left;

        for (const auto &frequency_type: default_frequency_type) {
            for (const auto &rate_heterogeneity: default_rate_heterogeneity) {
                const string model_descriptor = subst_model
                                                + frequency_type_label.at(frequency_type)
                                                + rate_heterogeneity_label.at(rate_heterogeneity);

                const auto normalized_model_descriptor = normalize_model_name(model_descriptor);
                LOG_DEBUG << "[" << index << "/" << num_models << "] Instantiating model " <<
                normalized_model_descriptor << endl;


                Model model(normalized_model_descriptor);

                for (auto p = 0U; p < msa.part_count(); ++p) {
                    msa.model(p, model);
                }

                TreeInfo treeinfo(options, tree, msa, tip_msa_idmap, part_assign);
                const auto num_branches = treeinfo.tree().num_branches();

                //LOG_DEBUG << "\tInitial LogLH = " << treeinfo.loglh(false) << std::endl;

                const double final_loglh = optimizer.optimize_model(treeinfo);

                LOG_INFO << "[" << index << "/" << num_models << "]\tModel " << std::setw(12) <<
                        normalized_model_descriptor << "\t\t" << final_loglh << endl;


                vector<PartitionModelEvaluation> partition_results(msa.part_count());


                for (auto p = 0; p < msa.part_count(); ++p) {
                    const double partition_loglh = treeinfo.pll_treeinfo().partition_loglh[p];
                    const size_t free_params = msa.model(p).num_free_params() + treeinfo.tree().num_branches();

                    /*
                     * My intuition is to set sample size to the length of the partition, i.e.
                     * sample_size = msa.part_info(p).length();
                     *
                     * However, ModelTest-NG uses the total sites.
                     */
                    const size_t sample_size = msa.total_sites();
                    ICScoreCalculator ic_score_calculator(free_params, sample_size);

                    partition_results[p].ic_criteria = ic_score_calculator.all(partition_loglh);
                    partition_results[p].model = msa.model(p);
                    partition_results[p].partition_loglh = partition_loglh;

                    LOG_DEBUG << "\tPartition #" << p << " BIC " << partition_results[p].ic_criteria[
                        InformationCriterion::bic] << endl;
                }

                results[normalized_model_descriptor] = partition_results;

                ++index;
            }
        }
    }


    LOG_INFO << endl;

    for (const auto &e: results) {
        LOG_DEBUG << "Model " << e.first << endl;
        for (auto p = 0U; p < msa.part_count(); ++p) {
            const auto part_result = e.second[p];
            LOG_DEBUG << "\t Partition #" << p << ": LogLH = " << part_result.partition_loglh << ", BIC =" <<
                    part_result.ic_criteria.at(InformationCriterion::bic) << ",  " << part_result.model.
                    to_string(true, 8) << endl;
        }
    }

    LOG_INFO << endl;
    const auto best_fit = choose_best_fit(results, InformationCriterion::bic);
    for (auto p = 0U; p < msa.part_count(); ++p) {
        LOG_INFO << "Partition #" << p << ": " << best_fit[p].model.to_string() << " BIC = " << best_fit[p].ic_criteria.
                at(InformationCriterion::bic) << endl;
    }
}

vector<PartitionModelEvaluation> ModelTest::choose_best_fit(const EvaluationResults &results,
                                                            InformationCriterion ic) const {
    const auto first = results.cbegin()->second;
    vector<PartitionModelEvaluation> best_fit{first};

    for (const auto &result: results) {
        const auto candidate_models = result.second;

        for (auto p = 0U; p < msa.part_count(); ++p) {
            if (candidate_models[p].ic_criteria.at(ic) < best_fit[p].ic_criteria.at(ic)) {

                best_fit[p] = candidate_models[p];
            }
        }
    }

    return best_fit;
}
