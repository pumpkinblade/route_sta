#include "log.hpp"
#include <chrono>

using clk = std::chrono::high_resolution_clock;
static clk::time_point start = clk::now();

double eplaseTime() {
  return std::chrono::duration<double>(clk::now() - start).count();
}
