//
// Created by christoph on 27/11/23.
//

// TODO: remove this debug file before merging with master!! Not meant for production!!
#include "debug.h"
#include <pll.h>
#include "io/file_io.hpp"
#include <string>
#include <openssl/sha.h>

void debug_to_newick_file(const pll_utree_t& tree, const char *fname) {
    std::string filename(fname);
    to_newick_file(tree, filename);
}

const unsigned char *readable_digest(const unsigned char *in, const int length) {
    unsigned char *readable = static_cast<unsigned char*>(malloc(length * 2 + 1));

    for (int i = 0; i < length; ++i) {
        char b = in[i];

        char upper_half = (b >> 4) & 0b1111;
        char lower_half = b & 0b1111;

        readable[2 * i + 0] = (upper_half < 10) ? '0' + upper_half : 'a' + (upper_half - 10);
        readable[2 * i + 1] = (lower_half < 10) ? '0' + lower_half : 'a' + (lower_half - 10);
    }
    readable[2 * length] = 0x00; // end string

    return readable;
}

const unsigned char *debug_hash_array(const void *arr, size_t length) {
    SHA256_CTX c;
    SHA256_Init(&c);

    unsigned char *digest = static_cast<unsigned char *>(malloc(SHA256_DIGEST_LENGTH));

    SHA256_Update(&c, arr, length );
    SHA256_Final(digest, &c);

    const unsigned char *readable = readable_digest(digest, SHA256_DIGEST_LENGTH);
    free(digest);
    return readable;
}

const unsigned char *debug_hash_partition_clv(const pll_partition_t * partition) {
    SHA256_CTX c;
    SHA256_Init(&c);

    for (int i = 0; i < partition->clv_buffers; ++i) {
        const size_t length = partition->sites * partition->states_padded * partition->rate_cats * sizeof(double);
        SHA256_Update(&c, static_cast<void *>(partition->clv[i]), length);

    }
    unsigned char *digest = static_cast<unsigned char *>(malloc(SHA256_DIGEST_LENGTH));
    SHA256_Final(digest, &c);

    const unsigned char *readable = readable_digest(digest, SHA256_DIGEST_LENGTH);
    free(digest);
    return readable;


}



void debug_array_to_file(const void *array, const size_t length, const char *fname) {
    FILE *f = fopen(fname, "a");

    size_t written = std::fwrite(array, 1, length, f);
    assert(written == length);

    std::fclose(f);
}

void debug_clvs_to_file(const pllmod_treeinfo_t *treeinfo, const char *fname) {
    assert(treeinfo->partition_count == 1);
    const pll_partition_t *partition = treeinfo->partitions[0];
    const size_t clv_length = partition->sites * partition->states_padded * partition->rate_cats * sizeof(double);

    const pll_utree_t& tree = *treeinfo->tree;

    char filename[1024];
    int rank;
    int size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    snprintf(filename, 1024, "%s_p%i_n%i_sites%i_states%i_rates%i-%libytes.clv",
            fname, rank, size, partition->sites, partition->states_padded, partition->rate_cats, clv_length);

    FILE *f = fopen(filename, "w");

    for (unsigned int i = tree.tip_count; i < tree.tip_count +tree.inner_count; i++) {
        const auto clv_index = tree.nodes[i]->clv_index;
        const double *clv = treeinfo->partitions[0]->clv[clv_index];

        size_t written = std::fwrite(clv, clv_length, 1, f);
    }
    std::fclose(f);

}
