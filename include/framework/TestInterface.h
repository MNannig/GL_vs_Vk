#pragma once

namespace framework {
class TestInterface
{
  public:
    virtual ~TestInterface() = default;

    static const unsigned int WINDOW_WIDTH = 1920;
    static const unsigned int WINDOW_HEIGHT = 1080;

    virtual void setup() = 0;
    virtual void run() = 0;
    virtual void teardown() = 0;
};
}
