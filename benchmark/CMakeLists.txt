find_package(benchmark)

add_executable(stats_benchmark bm_stats.cpp)
target_include_directories(stats_benchmark PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(stats_benchmark PRIVATE benchmark::benchmark ${LIB_NAME})
