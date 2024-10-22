#ifndef __LOG_HPP__
#define __LOG_HPP__

#include <cstdio>

double eplaseTime();

#define ANSI_FG_RED "\33[1;31m"
#define ANSI_FG_GREEN "\33[1;32m"
#define ANSI_FG_YELLOW "\33[1;33m"
#define ANSI_FG_BLUE "\33[1;34m"
#define ANSI_NONE "\33[0m"

#define LOG_ERROR(fmt, ...)                                                    \
  do {                                                                         \
    std::printf(ANSI_FG_RED "[%.3f] ERROR: [%s:%d] " fmt ANSI_NONE "\n",       \
                eplaseTime(), __FILE__, __LINE__, ##__VA_ARGS__);              \
  } while (0)

#define LOG_WARN(fmt, ...)                                                     \
  do {                                                                         \
    std::printf(ANSI_FG_YELLOW "[%.3f] WARN: [%s:%d] " fmt ANSI_NONE "\n",     \
                eplaseTime(), __FILE__, __LINE__, ##__VA_ARGS__);              \
  } while (0)

#define LOG_INFO(fmt, ...)                                                     \
  do {                                                                         \
    std::printf(ANSI_FG_GREEN "[%.3f] INFO: [%s:%d] " fmt ANSI_NONE "\n",      \
                eplaseTime(), __FILE__, __LINE__, ##__VA_ARGS__);              \
  } while (0)

#define LOG_DEBUG(fmt, ...)                                                    \
  do {                                                                         \
    std::printf(ANSI_FG_BLUE "[%.3f] DEBUG: [%s:%d] " fmt ANSI_NONE "\n",      \
                eplaseTime(), __FILE__, __LINE__, ##__VA_ARGS__);              \
  } while (0)

#define LOG_TRACE(fmt, ...)                                                    \
  do {                                                                         \
    std::printf("[%.3f] TRACE: [%s:%d] " fmt "\n", eplaseTime(), __FILE__,     \
                __LINE__, ##__VA_ARGS__);                                      \
  } while (0)

#define ASSERT(x, fmt, ...)                                                    \
  do {                                                                         \
    if (!(x)) {                                                                \
      LOG_ERROR(fmt, ##__VA_ARGS__);                                           \
      std::exit(-1);                                                           \
    }                                                                          \
  } while (0)

#define PANIC(fmt, ...) ASSERT(0, fmt, ##__VA_ARGS__)

#endif
