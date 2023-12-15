//
// Created by christoph on 27/11/23.
//

#ifndef RAXML_NG_DEBUG_H
#define RAXML_NG_DEBUG_H

#include <libpll/pll_tree.h>
#include <pll.h>

/* Use char array because that can be created by GDB */
void debug_to_newick_file(const pll_utree_t& tree, const char *fname);
const unsigned char *debug_hash_array(double *arr, size_t length);
const unsigned char *debug_hash_partition_clv(const pll_partition_t * partition);
void debug_clvs_to_file(const pllmod_treeinfo_t *treeinfo, const char *fname);
void debug_array_to_file(const void *array, const size_t length, const char *fname);

#endif //RAXML_NG_DEBUG_H
