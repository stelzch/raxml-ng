#ifdef _RAXML_JSON
#ifndef RAXML_IO_JSON_HPP_
#define RAXML_IO_JSON_HPP_

#include "../Options.hpp"
#include "../PartitionedMSA.hpp"
#include "../modeltest/ModelTest.hpp"
#include "../Checkpoint.hpp"
#include "../version.h"

/* Need global state to signal that JSON on stdout is desired, because Options are not available in error handlers */
extern bool RAXML_JSON_STDOUT;

void print_json(const Options& opts, const PartitionedMSA *msa, const CheckpointFile& checkp, const ModelTest *modeltest, const DifficultyPredictor *difficulty_predictor, const RFDistCalculator *dist_calculator, double used_wh);
void print_error_json(const Options *opts, std::string error_type, std::string error_message, std::map<std::string, std::string> additional_info = {});

#endif
#endif
