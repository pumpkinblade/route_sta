#ifndef __UTILS_HPP__
#define __UTILS_HPP__

#include <cstdio>

#define ANSI_FG_RED "\33[1;31m"
#define ANSI_FG_GREEN "\33[1;32m"
#define ANSI_FG_YELLOW "\33[1;33m"
#define ANSI_FG_BLUE "\33[1;34m"
#define ANSI_NONE "\33[0m"

#define LOG_ERROR(fmt, ...)                                                    \
  std::printf(ANSI_FG_RED "ERROR: [%s:%d] " fmt ANSI_NONE "\n", __FILE__,      \
              __LINE__, ##__VA_ARGS__)
#define LOG_WARN(fmt, ...)                                                     \
  std::printf(ANSI_FG_YELLOW "WARN: [%s:%d] " fmt ANSI_NONE "\n", __FILE__,    \
              __LINE__, ##__VA_ARGS__)
#define LOG_INFO(fmt, ...)                                                     \
  std::printf(ANSI_FG_GREEN "INFO: [%s:%d] " fmt ANSI_NONE "\n", __FILE__,     \
              __LINE__, ##__VA_ARGS__)
#define LOG_DEBUG(fmt, ...)                                                    \
  std::printf(ANSI_FG_BLUE "DEBUG: [%s:%d] " fmt ANSI_NONE "\n", __FILE__,     \
              __LINE__, ##__VA_ARGS__)
#define LOG_TRACE(fmt, ...)                                                    \
  std::printf("TRACE: [%s:%d] " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__)

#define ASSERT(x, fmt, ...)                                                    \
  do {                                                                         \
    if (!(x)) {                                                                \
      LOG_ERROR(fmt, ##__VA_ARGS__);                                           \
      std::exit(-1);                                                           \
    }                                                                          \
  } while (0)

#define PANIC(fmt, ...) ASSERT(0, fmt, ##__VA_ARGS__)

#endif
