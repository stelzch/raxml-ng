#include <vector>
#include <mpi.h>
#include <string>
#include "RaxmlTest.hpp"
#include "src/ParallelContext.hpp"

#include "src/io/file_io.hpp"

TEST(ReproducibilityTest, loglh) {
    int rank, comm_size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &comm_size);
    //attach_debugger(rank == 0);

    const int SITES = 460;
    const int TIPS = 354;
    const std::string DATA_DIR = "/home/christoph/Studium/HiWi2/raxml-ng/test/src/data";

    const auto msa_path = DATA_DIR + "/DNA-Data/354/354.phy";
    const auto tree_path = DATA_DIR + "starttree_354.newick";

    PhylipStream msa_stream(msa_path);
    MSA msa;
    msa_stream >> msa;

    char tip_names[TIPS][20];
    char *tip_name_ptr[TIPS];
    for (int i = 0; i < TIPS; i++) {
        snprintf(tip_names[i], 8, "tip%d", i);
        tip_name_ptr[i] = &tip_names[i][0];
    }



    const auto tree = Tree::buildRandom(TIPS, tip_name_ptr, 42);

    EXPECT_EQ(msa.num_sites(), SITES);
    EXPECT_EQ(tree.num_tips(), TIPS);


    //EXPECT_EQ(comm_size, 2);
}
