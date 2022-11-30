#include <stdexcept>
#include <gtest/gtest.h>
#include <string>
#include <iostream>
#include <mpi.h>

#include "RaxmlTest.hpp"
#include "gtest-mpi-listener.hpp"

RaxmlTest* env;

int main(int argc, char** argv)
{
  env = new RaxmlTest();

  // Set data dir using the program path.
  std::string call = argv[0];
  std::size_t found = call.find_last_of("/\\");
  if (found != std::string::npos) {
      env->data_dir = call.substr(0,found) + "/../data/";
  }

//  env->tree_file = std::string(env->data_dir);
//  env->tree_file += "ref.tre";

//  env->out_dir  = std::string("/tmp/epatest/");
//  std::string cmd("mkdir ");
//  cmd += env->out_dir.c_str();
//  system(cmd.c_str());

  ::testing::InitGoogleTest(&argc, argv);

  MPI_Init(&argc, &argv);
  int init_flag;
  MPI_Initialized(&init_flag);
  if (!init_flag) {
      throw std::runtime_error("MPI not initialized");
  }

  ::testing::AddGlobalTestEnvironment(new GTestMPIListener::MPIEnvironment);
  ::testing::TestEventListeners& listeners = ::testing::UnitTest::GetInstance()->listeners();
  ::testing::TestEventListener* l = listeners.Release(listeners.default_result_printer());
  listeners.Append(new GTestMPIListener::MPIWrapperPrinter(l, MPI_COMM_WORLD));

  ::testing::AddGlobalTestEnvironment(env);
  auto result = RUN_ALL_TESTS();

  return result;
}
