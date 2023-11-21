#ifndef __CAP_DEBUG__
#define __CAP_DEBUG__
#include <errno.h>
#include <string.h>
#include <thread>
#include <unistd.h>

#include <fmt/core.h>

#define LEVEL_TRACE (1)
#define LEVEL_DEBUG (2)
#define LEVEL_INFO  (3)
#define LEVEL_WARN  (4)
#define LEVEL_ERROR (5)
#define LEVEL_OFF   (6)

#ifndef ACTIVE_LEVEL
#define ACTIVE_LEVEL (LEVEL_OFF)
#endif

#define PRINT(inputstr, type, ...) \
    do { \
    fmt::print("[{}:{}:{}]: " inputstr "\n", __FILE__, __LINE__, \
               type, ##__VA_ARGS__); \
    fflush(stdout); \
    } while (0)

#if ACTIVE_LEVEL <= LEVEL_TRACE
#define TRACE(str, ...) PRINT(str, "TRACE", ##__VA_ARGS__)
#else
#define TRACE(str, ...) (void(0))
#endif

#if ACTIVE_LEVEL <= LEVEL_DEBUG
#define DEBUG(str, ...) PRINT(str, "DEBUG", ##__VA_ARGS__)
#else
#define DEBUG(str, ...) (void(0))
#endif

#if ACTIVE_LEVEL <= LEVEL_INFO
#define INFO(str, ...) PRINT(str, "INFO", ##__VA_ARGS__)
#else
#define INFO(str, ...) (void(0))
#endif

#if ACTIVE_LEVEL <= LEVEL_WARN
#define WARN(str, ...) PRINT(str, "WARN", ##__VA_ARGS__)
#else
#define WARN(str, ...) (void(0))
#endif

#if ACTIVE_LEVEL <= LEVEL_ERROR
#define ERROR(str, ...) PRINT(str, "ERROR", ##__VA_ARGS__)
#define ERR(str, ...) PRINT(str, "ERROR", ##__VA_ARGS__)
#else
#define ERROR(str, ...) (void(0))
#define ERR(str, ...) (void(0))
#endif

#endif // __CAP_DEBUG__

