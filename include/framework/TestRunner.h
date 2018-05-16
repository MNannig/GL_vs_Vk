#pragma once

#include <base/ArgumentParser.h>
#include <framework/BenchmarkableTest.h>

#include <memory>

namespace framework {
class TestRunner
{
  public:
    TestRunner(base::ArgumentParser argumentParser);

    int run();

  private:
    int run_gl(int testNumber, bool multithreaded, bool benchmarkMode, int benchmark_stat, float benchmarkTime, double testStartTime, int n, int nt);
    int run_vk(int testNumber, bool multithreaded, bool benchmarkMode, int benchmark_stat, float benchmarkTime, double testStartTime, int n, int nt);
    int run_any(std::unique_ptr<BenchmarkableTest> test, int benchmark_stat, double testStartTime);

    base::ArgumentParser arguments;
};
}
