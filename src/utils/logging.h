#ifndef LOGGING_H
#define LOGGING_H

#include <stdbool.h>

// Log levels
typedef enum {
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARN,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_FATAL
} log_level_t;

// Initialize logging system
bool logging_init(const char* log_file);

// Clean up logging resources
void logging_cleanup(void);

// Set minimum log level
void logging_set_level(log_level_t level);

// Logging functions
void log_debug(const char* format, ...);
void log_info(const char* format, ...);
void log_warn(const char* format, ...);
void log_error(const char* format, ...);
void log_fatal(const char* format, ...);

// Helper macro for logging with file and line information
#define LOG_DEBUG_FMT(fmt, ...) log_debug("[%s:%d] " fmt, __FILE__, __LINE__, ##__VA_ARGS__)
#define LOG_INFO_FMT(fmt, ...) log_info("[%s:%d] " fmt, __FILE__, __LINE__, ##__VA_ARGS__)
#define LOG_WARN_FMT(fmt, ...) log_warn("[%s:%d] " fmt, __FILE__, __LINE__, ##__VA_ARGS__)
#define LOG_ERROR_FMT(fmt, ...) log_error("[%s:%d] " fmt, __FILE__, __LINE__, ##__VA_ARGS__)
#define LOG_FATAL_FMT(fmt, ...) log_fatal("[%s:%d] " fmt, __FILE__, __LINE__, ##__VA_ARGS__)

#endif // LOGGING_H 