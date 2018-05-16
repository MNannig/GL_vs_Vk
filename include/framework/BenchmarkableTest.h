#pragma once

#include <framework/TestInterface.h>

#include <cstddef>

namespace framework {
class BenchmarkableTest : public TestInterface
{
  public:
    BenchmarkableTest(bool benchmarkMode, int benchmark_stat, float benchmarkTime, int n, int nt);
    BenchmarkableTest(bool benchmarkMode, float benchmarkTime);
    virtual ~BenchmarkableTest() = default;

    virtual void printStatistics(int benchmark_stat) const;
    void startMeasuring();
    void startMeasuring(double startTime);

    static double getCurrentTime();

  protected:
    bool processFrameTime();
    bool processFrameTime(double frameTime);

    bool _benchmarkEnabled;
    bool _firstSecondIgnored;
    int _benchmark_stat;
    double _benchmarkTime;
    double _minFrameTime;
    double _maxFrameTime;
    double _startTime;
    double _lastMeasureTime;
    double _measuredTime;
    int _n;
    int _nt;
    std::size_t _frameCount;
};
}
