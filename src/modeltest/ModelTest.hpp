#ifndef MODELTEST_HPP_
#define MODELTEST_HPP_
#include "../TreeInfo.hpp"
#include "../Optimizer.hpp"

struct PartitionModelEvaluation {
    Model model;
    double partition_loglh;
    std::map<InformationCriterion, double> ic_criteria;
};


class ModelTest {
public:
    ModelTest(const Options &options, PartitionedMSA &msa, const Tree &tree, const IDVector &tip_msa_idmap,
              const PartitionAssignment &part_assign, Optimizer &optimizer);

    void optimize_model();

private:
    const Options &options;
    PartitionedMSA &msa;
    const Tree &tree;
    const IDVector &tip_msa_idmap;
    const PartitionAssignment &part_assign;
    Optimizer &optimizer;

    /// map from model descriptor to per-partition evaluation results
    using EvaluationResults = vector<vector<PartitionModelEvaluation> >;


    static void sort_by_score(EvaluationResults &results, InformationCriterion ic, unsigned int partition_idx);

    static void print_xml(ostream &os, EvaluationResults &results);
};


#endif //MODELTEST_HPP_
