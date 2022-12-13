#include <iostream>
#include <mpi.h>
#include <string>
#include "RaxmlTest.hpp"
#include "src/ParallelContext.hpp"
#include "src/TreeInfo.hpp"

#include "src/io/file_io.hpp"
#include "src/loadbalance/LoadBalancer.hpp"

TEST(ReproducibilityTest, loglh) {
    int rank, comm_size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &comm_size);
    //
    //attach_debugger(rank == 1);

    const int SITES = 460;
    const int TIPS = 354;

    const std::string msa_path = std::string(TEST_DATA_DIRECTORY) + "/354.phy";

    std::cout << "Loading MSA from '" << msa_path << "'" << std::endl;


    PhylipStream msa_stream(msa_path);
    MSA msa;
    msa_stream >> msa;


    // Construct partitioned MSA
    PartitionedMSA partitioned_msa;
    partitioned_msa.emplace_part_info("partition_1", DataType::dna, "GTR", "");
    partitioned_msa.part_msa(0, std::move(msa));

    PartitionAssignment part_assign;
    part_assign.assign_sites(0, 0, SITES);

    // Construct random tree
    char tip_names[TIPS][20];
    char *tip_name_ptr[TIPS];
    for (int i = 0; i < TIPS; i++) {
        snprintf(tip_names[i], 8, "tip%d", i);
        tip_name_ptr[i] = &tip_names[i][0];
    }

    const auto tree = Tree::buildRandom(TIPS, tip_name_ptr, 42);

    EXPECT_EQ(partitioned_msa.total_sites(), SITES);
    EXPECT_EQ(tree.num_tips(), TIPS);
    EXPECT_EQ(partitioned_msa.part_count(), 1);

    double expected_loglh;
    for (int participating_processors = 1; participating_processors <= comm_size; ++participating_processors) {
        MPI_Bcast(&expected_loglh, 1, MPI_DOUBLE,
                0, MPI_COMM_WORLD);

        // 0. Create  communicator
        bool participating = (rank < participating_processors);
        int group = participating ? 1 : 0;
        MPI_Comm sub_communicator;
        int ret = MPI_Comm_split(MPI_COMM_WORLD, group, 0, &sub_communicator);
        EXPECT_EQ(ret, 0);

        if (!participating) {
            continue;
        }

        int sub_rank;
        MPI_Comm_rank(sub_communicator, &sub_rank);

        // 1. Reinitialize ParallelContext
        EXPECT_EQ(ParallelContext::num_threads(), 1);
        ParallelContext::init_mpi(0, nullptr, &sub_communicator);
        set_default_reduction_context_communicator(sub_communicator);


        // 2. Assign sites to participating processors
        BenoitLoadBalancer load_balancer;
        auto proc_part_assignment = load_balancer.get_all_assignments(part_assign, participating_processors);


        // 3. Construct TreeInfo
        IDVector id_vector;
        Options opts;
        opts.num_ranks = participating_processors;


        TreeInfo tree_info(opts, tree, partitioned_msa, id_vector, proc_part_assignment.at(sub_rank));

        // 4. Calculate LogLH score
        double computed_loglh = tree_info.loglh();

        // 5. Make sure scores are consistent across processors in this cluster
        double root_computed_loglh = computed_loglh;;
        MPI_Bcast(&root_computed_loglh, 1, MPI_DOUBLE,
                0, sub_communicator);

	MPI_Comm_free(&sub_communicator);

        EXPECT_EQ(computed_loglh, root_computed_loglh);

        if (participating_processors == 1) {
            // 6a. Set the expected value in the first iteration
            expected_loglh = computed_loglh;
        } else {
            // 6b. Compare against previous result otherwise, expect them to be equal
            EXPECT_EQ(computed_loglh, expected_loglh);
        }
    }


}
