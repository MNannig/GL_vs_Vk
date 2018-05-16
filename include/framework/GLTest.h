#pragma once

#include <base/gl/Window.h>
#include <framework/BenchmarkableTest.h>

namespace framework{
class GLTest : public BenchmarkableTest
{
  public:
    GLTest(const std::string& testName, bool benchmarkMode, int benchmark_stat, float benchmarkTime, int n, int nt);
    GLTest(const std::string& testName, bool benchmarkMode, float benchmarkTime);
    virtual ~GLTest() = default;

    virtual void setup() override;
    virtual void teardown() override;

    void printStatistics(int benchmark_stat) const override;

  protected:
    base::gl::Window window_;
};
}
