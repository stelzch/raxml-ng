#ifdef _RAXML_JSON
#include "json.hpp"

#include "../autotune/ResourceEstimator.hpp"


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

json evaluate_json(const CheckpointFile &checkp) {
    json a = json::array();

    for (const auto &tree : checkp.ml_trees) {
      const double lnL = tree.second.first;
      a.push_back({
        {"lnL", format_loglh(lnL)},
      });
    }

    return a;
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
      {"num_threads", opts.num_threads},
      {"num_workers", opts.num_workers},
      {"opt_freerate", opts.free_rate_opt_method_short_name()},
      {"opt_pattern_compression", opts.use_pattern_compression},
      {"opt_rate_scalers", opts.use_rate_scalers},
      {"opt_simd", opts.simd_arch_name()},
      {"opt_site_repeats", opts.use_repeats},
      {"opt_tip_inner", opts.use_tip_inner},
      {"random_seed", opts.random_seed},
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
    j["evaluate"] = evaluate_json(checkp);
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
