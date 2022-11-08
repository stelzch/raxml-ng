set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_library(summation STATIC 
    binary_tree.cpp
    summation_strategy.cpp
    util.cpp)
target_compile_options(summation PRIVATE -mavx)
