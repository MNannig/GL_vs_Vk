#pragma once

#include <base/vkx/Application.h>
#include <framework/BenchmarkableTest.h>

#include <string>

namespace framework {
class VKTest : public BenchmarkableTest, public base::vkx::Application
{
  public:
    VKTest(const std::string& testName, bool benchmarkMode, int benchmark_stat, float benchmarkTime, int n, int nt);
    VKTest(const std::string& testName, bool benchmarkMode, float benchmarkTime);
    virtual ~VKTest() = default;

    virtual void setup() override;
    virtual void teardown() override;

    void printStatistics(int benchmark_stat) const override;

  protected:
    //
};
}
