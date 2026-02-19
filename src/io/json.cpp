#ifdef _RAXML_JSON
#include "json.hpp"

#include "../autotune/ResourceEstimator.hpp"
#include "../ICScoreCalculator.hpp"


#include <nlohmann/json.hpp>
using json = nlohmann::json;


const char *JSON_SCHEMA = "https://raxml.ng/schema/raxml-ng-schema-v1.json";
const char *JSON_FORMAT_VERSION = "1";
bool RAXML_JSON_STDOUT = false;

string format_loglh(const double v) {
    std::ostringstream out;
    out << std::fixed << FMT_LH(v);
    return std::move(out).str();
}

json moose_json(const Options &options, const ModelTest *modeltest, const PartitionedMSA *msa) {
  auto results = modeltest->get_results();
  if (results.empty()) {
    return {};
  }

  const auto datatype = options.data_type;
  const auto datatype_name = results.at(0).at(0)->model.data_type_name();

  vector<string> frequencies;
  frequencies.reserve(default_frequency_type.size());
  for (auto f : default_frequency_type) {
    frequencies.push_back(frequency_type_label(datatype, f));
  }

  vector<string> rhas_labels;
  rhas_labels.reserve(options.modeltest_rhas.size());
  for (auto rhas : options.modeltest_rhas) {
    rhas_labels.push_back(rate_heterogeneity_label[static_cast<size_t>(rhas)]);
  }

  json heuristics;
  if (options.modeltest_heuristics.find(HeuristicType::RHAS) != options.modeltest_heuristics.cend()) {
    heuristics["RHAS"] = {
      {"ic_delta", options.modeltest_significant_ic_delta},
      {"mode", options.modeltest_rhas_heuristic_mode == RHASHeuristicMode::AllSignficantCategoryCounts ? "all significant" : "only optimal"}
    };
  }
  if (options.modeltest_heuristics.find(HeuristicType::FREERATE) != options.modeltest_heuristics.cend()) {
    heuristics["freerate"] = json::object();
  }

  auto tree = BasicTree(msa->taxon_count());
  auto num_branches = tree.num_branches();

  auto best_fit = json::array();
  auto evaluation_results = json::array();
  for (auto p = 0U; p < results.size(); ++p) {
    const auto &best_model = results[p][0]->model;

    const auto &part = msa->part_list().at(p);

    best_fit.push_back({
      {"partition_name", part.name()},
      {"sites", part.range_string()},
      {"model", best_model.to_string()},
      {"model_params", best_model.to_string(true, logger().precision(LogElement::model))}
    });

    json evaluations;
    for (auto result : results[p]) {
      const auto &model = result->model;
      evaluations.push_back({
        {"model", model.to_string()},
        {"model_params", model.to_string(true, logger().precision(LogElement::model))},
        {"free_params", num_branches + model.num_free_params()},
        {"ic_score", format_loglh(result->ic_score)},
        {"lnL", format_loglh(result->loglh)}
      });
    }
    evaluation_results.push_back(evaluations);
  }

  json candidate_selection;
  const auto matrices = options.modeltest_subst_models.empty() ? moose_matrix_names(options.data_type)
                                                                : options.modeltest_subst_models;
  candidate_selection[datatype_name] = {
      {"substitution_matrix", matrices},
      {"frequency", frequencies},
      {"rhas", {
        {"type", rhas_labels},
        {"min_freerate_categories", options.free_rate_min_categories},
        {"max_freerate_categories", options.free_rate_max_categories},
        {"gamma_categories", 4}
      }}
  };

  return {
    {"ic_criterion", options.ic_name()},
    {"lh_epsilon", format_loglh(options.lh_epsilon)},
    {"candidate_selection", candidate_selection},
    {"heuristics", heuristics},
    {"best_fit", best_fit},
    {"results", evaluation_results}
  };

}

json parse_command_json(const Options &opts, const PartitionedMSA &parted_msa) {
  StaticResourceEstimator resEstimator(parted_msa, opts);
  const auto res = resEstimator.estimate();

  return {
    {"taxon_clv_size", res.taxon_clv_size},
    {"memory_estimate_bytes", res.total_mem_size},
    {"threads_balanced", res.num_threads_balanced},
    {"threads_throughput", res.num_threads_throughput},
    {"threads_response", res.num_threads_response},
  };
}

json pythia_json(const DifficultyPredictor *difficulty_predictor) {
    return {
        {"difficulty", difficulty_predictor->difficulty()},
        {"features", {
            {"avg_rf_dist", difficulty_predictor->avg_rrf()},
            {"prop_unique_topologies", difficulty_predictor->prop_uniq()},
            {"taxa", difficulty_predictor->features()->taxa},
            {"sites", difficulty_predictor->features()->sites},
            {"patterns_per_site", difficulty_predictor->features()->patterns_per_site},
            {"patterns_per_taxa", difficulty_predictor->features()->patterns_per_taxa},
            {"sites_per_taxa", difficulty_predictor->features()->sites_per_taxa},
            {"proportion_gaps", difficulty_predictor->features()->proportion_gaps},
            {"proportion_invariant", difficulty_predictor->features()->proportion_invariant},
            {"entropy", difficulty_predictor->features()->entropy},
            {"pattern_entropy", difficulty_predictor->features()->pattern_entropy},
            {"bollback_multinomial", difficulty_predictor->features()->bollback_multinomial},
        }},
    };
}

json rfdist_json(const RFDistCalculator *dist_calculator) {
    return {
        {"average_absolute_rfdist", dist_calculator->avg_rf()},
        {"average_relative_rfdist", dist_calculator->avg_rrf()},
        {"num_topologies", dist_calculator->num_trees()},
        {"num_unique_topologies", dist_calculator->num_uniq_trees()},
    };
}

json evaluate_json(const Options &opts, const PartitionedMSA *msa, const CheckpointFile &checkp) {
  json ml_trees = json::array();
  for (const auto &tree : checkp.ml_trees) {
    const double lnL = tree.second.first;
    ml_trees.push_back({
      {"lnL", format_loglh(lnL)},
    });
  }


  json best_fit_model = json::array();
  for (const auto &entry :  checkp.best_tree().models) {
    const auto &part = msa->part_info(entry.first);
    const auto &model = entry.second;
    best_fit_model.push_back({
      {"partition_name", part.name()},
      {"sites", part.range_string()},
      {"model", model.to_string()},
      {"model_params", model.to_string(true, logger().precision(LogElement::model))}
    });
  }

  const double best_lnL = checkp.best_tree().loglh;

  ICScoreCalculator ic_calc(msa->total_free_params(opts.brlen_linkage), msa->total_sites());
  auto ic_scores = ic_calc.all(best_lnL);

  return {
    {"best", {
      {"lnL", format_loglh(best_lnL)},
      {"best_fit", best_fit_model},
      {"aic", format_loglh(ic_scores[InformationCriterion::aic])},
      {"aicc", format_loglh(ic_scores[InformationCriterion::aicc])},
      {"bic", format_loglh(ic_scores[InformationCriterion::bic])},
    }},
    {"ml_trees", ml_trees},
  };
}


NLOHMANN_JSON_SERIALIZE_ENUM(TopologyOptMethod, {
  {TopologyOptMethod::none, "none"},
  {TopologyOptMethod::classic, "classic"},
  {TopologyOptMethod::adaptive, "adaptive"},
  {TopologyOptMethod::rapidBS, "rapidBS"},
  {TopologyOptMethod::nniRound, "nniRound"},
  {TopologyOptMethod::simplified, "simplified"},
  {TopologyOptMethod::adafast, "adafast"}
});

NLOHMANN_JSON_SERIALIZE_ENUM(FreerateOptMethod, {
  {FreerateOptMethod::AUTO, "auto"},
  {FreerateOptMethod::EM_BFGS, "em-bfgs"},
  {FreerateOptMethod::EM_BRENT, "em-brent"},
  {FreerateOptMethod::LBFGSB, "bfgs-bfgs"}
});

NLOHMANN_JSON_SERIALIZE_ENUM(StoppingRule, {
  {StoppingRule::none, "none"},
  {StoppingRule::sn_rell, "sn_rell"},
  {StoppingRule::sn_normal, "sn_normal"},
  {StoppingRule::kh, "kh"},
  {StoppingRule::kh_mult, "kh_mult"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(BranchSupportMetric, {
  {BranchSupportMetric::fbp, "fbp"},
  {BranchSupportMetric::tbe, "tbe"},
  {BranchSupportMetric::rbs, "rbs"},
  {BranchSupportMetric::sh_alrt, "sh_alrt"},
  {BranchSupportMetric::ebg, "ebg"},
  {BranchSupportMetric::ps, "ps"},
  {BranchSupportMetric::pbs, "pbs"},
  {BranchSupportMetric::ic1, "ic1"},
  {BranchSupportMetric::ica, "ica"},
  {BranchSupportMetric::gcf, "gcf"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(StartingTree, {
  {StartingTree::random, "random"},
  {StartingTree::parsimony, "parsimony"},
  {StartingTree::user, "user"},
  {StartingTree::adaptive, "adaptive"},
  {StartingTree::consensus, "consensus"}
});

json options_json(const Options &opts) {
  json j {
      {"opt_freerate", opts.free_rate_opt_method_short_name()},
      {"opt_pattern_compression", opts.use_pattern_compression},
      {"opt_rate_scalers", opts.use_rate_scalers},
      {"opt_simd", opts.simd_arch_name()},
      {"opt_site_repeats", opts.use_repeats},
      {"opt_tip_inner", opts.use_tip_inner},
      {"num_threads", opts.num_threads},
      {"num_workers", opts.num_workers},
      {"random_seed", opts.random_seed},
  };
  j["command"] = CommandNames[static_cast<unsigned int>(opts.command)];

  if ((opts.command == Command::search || opts.command == Command::bootstrap ||
      opts.command == Command::all) &&
      opts.topology_opt_method != TopologyOptMethod::none) {
    j["topology_opt_method"] = opts.topology_opt_method;
  }

  if (opts.command == Command::bootstrap || opts.command == Command::all ||
      opts.command == Command::support)
  {
    j["branch_support_metric"] = json::array();
    for (const auto &metric : opts.bs_metrics) {
      j["branch_support_metric"].push_back(metric);
    }
  }

  auto start_trees = json::array();
  for (const auto &t : opts.start_trees) {
    const auto &type = t.first;
    const auto &count = t.second;
    start_trees.push_back({ {"type", type} });

    if (type == StartingTree::random) {
      start_trees.back()["count"] = count;
    }

    if (type == StartingTree::parsimony) {
      start_trees.back()["count"] = count;
      start_trees.back()["parsimony_spr"] = opts.use_pars_spr;
      start_trees.back()["parsimony-brlen"] = opts.use_pars_brlen;
    }

  }
  j["starting_trees"] = start_trees;

  return j;
}

void print_json(const Options& opts, const PartitionedMSA *msa, const CheckpointFile& checkp, const ModelTest *modeltest, const DifficultyPredictor *difficulty_predictor, const RFDistCalculator *dist_calculator, double used_wh) {
  if (opts.json_file().empty()) {
    return;
  }

  json j = {
    {"$schema", JSON_SCHEMA},
    {"$version", JSON_FORMAT_VERSION},
    {"metadata", {
      {"elapsed_time", global_timer().elapsed_seconds()},
      {"elapsed_time_total", checkp.elapsed_seconds + global_timer().elapsed_seconds()},
      {"invocation", opts.cmdline},
      {"invocation_time", sysutil_fmt_time(global_timer().start_time())},
      {"release_date", RAXML_DATE},
      {"release", RAXML_VERSION},
      {"system_cpu_cores", sysutil_get_cpu_cores()},
      {"system_cpu_model", sysutil_get_cpu_model()},
      {"system_mem_total_bytes", sysutil_get_memtotal()},
    }},
  };

  if (used_wh > 0) {
    j["metadata"]["used_wh"] = used_wh;
  }

  j["options"] = options_json(opts);

  if(msa != nullptr) {
    j["alignment"] = {
      {"taxa", msa->taxon_count()},
      {"sites", msa->total_sites()},
      {"patterns", msa->total_patterns()},
      {"num_partitions", msa->part_count()},
    };

    if (msa->difficulty_score() >= 0) {
      j["alignment"]["difficulty"] = msa->difficulty_score();
    }
  }

  if (opts.auto_model() && modeltest != nullptr) {
    j["moose"] = moose_json(opts, modeltest, msa);
  }

  if (opts.command == Command::evaluate || opts.command == Command::sitelh ||
      opts.command == Command::ancestral || opts.command == Command::mutmap) {
    j["evaluate"] = evaluate_json(opts, msa, checkp);
  }

  if (opts.command == Command::search || opts.command == Command::all) {
    j["search"] = evaluate_json(opts, msa, checkp);
  }

  if (opts.command == Command::parse) {
    j["parse"] = parse_command_json(opts, *msa);
  }

  if (difficulty_predictor != nullptr) {
    j["pythia"] = pythia_json(difficulty_predictor);
  }
  
  if (dist_calculator != nullptr) {
    j["rfdist"] = rfdist_json(dist_calculator);
  }

  std::ofstream of(opts.json_file());
  of << j << "\n";

  if (RAXML_JSON_STDOUT) {
    std::cout << j << "\n";
  }
}

void print_error_json(const Options *opts, const std::string error_type, const std::string error_message, std::map<std::string, std::string> additional_info) {
  json j = {
    {"$schema", JSON_SCHEMA},
    {"$version", JSON_FORMAT_VERSION},
    {"error", {
      {"message", error_message},
      {"type", error_type},
    }}};

  if (!additional_info.empty()) {
    j["error"]["props"] = json(additional_info);
  }

  if (opts != nullptr && !opts->json_file().empty()) {
    std::ofstream of(opts->json_file());
    of << j << "\n";
  }

  if (RAXML_JSON_STDOUT) {
    std::cout << j << "\n";
  }
}

#endif
