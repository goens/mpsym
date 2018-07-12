#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <random>
#include <string>
#include <vector>

#include "arch_graph.h"
#include "profile_utility.h"

#define RUNS 100

using cgtl::ArchGraph;
using cgtl::TaskMapping;

static long compare_mappings(ArchGraph const &ag, std::size_t num_tasks)
{
  static std::default_random_engine re(time(nullptr));

  std::uniform_int_distribution<std::size_t> dist(0u, ag.num_processors() - 1u);

  std::vector<std::size_t> task_mapping1(num_tasks);
  std::vector<std::size_t> task_mapping2(num_tasks);

  long exec_ticks = 0;

  for (int run = 0; run < RUNS; ++run) {
    for (std::size_t i = 0u; i < num_tasks; ++i) {
      task_mapping1[i] = dist(re);
      task_mapping2[i] = dist(re);
    }

    auto t0 = std::chrono::high_resolution_clock::now();

    TaskMapping tm1 = ag.mapping(task_mapping1);
    TaskMapping tm2 = ag.mapping(task_mapping2);

    volatile bool result = ag.equivalent(tm1, tm2);
    (void) result;

    exec_ticks += std::chrono::duration_cast<std::chrono::microseconds>(
      std::chrono::high_resolution_clock::now() - t0).count();
  }

  return exec_ticks;
}

int main()
{
  ArchGraph ag;

  std::vector<std::string> arch_graphs {
    resource_path("mcsoc.lua")
  };

  long exec_ticks = 0;

  for (auto const &path : arch_graphs) {
    ag.fromlua(path);
    ag.generate_automorphisms();

    std::size_t num_tasks = ag.num_processors();
    while (num_tasks >>= 1u) {
      printf("Comparing mappings (%zu tasks on %zu processors, %d mappings)\n",
             num_tasks, ag.num_processors(), RUNS);

      exec_ticks += compare_mappings(ag, num_tasks);
    }
  }

  printf("Total execution time: %ld microseconds\n", exec_ticks);
}
