
add_executable(bench bench.cpp)
add_dependencies(bench benchmark benchmark_main)

target_link_libraries(bench PUBLIC benchmark benchmark_main)
target_link_libraries(bench PUBLIC bee log)
