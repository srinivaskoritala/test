#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <pthread.h>
#include "logging.h"

// Global variables
static FILE* log_file = NULL;
static log_level_t min_level = LOG_LEVEL_INFO;
static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

// Log level strings
static const char* level_strings[] = {
    "DEBUG",
    "INFO",
    "WARN",
    "ERROR",
    "FATAL"
};

// Initialize logging system
bool logging_init(const char* log_file_path) {
    if (!log_file_path) return false;

    log_file = fopen(log_file_path, "a");
    if (!log_file) {
        fprintf(stderr, "Failed to open log file: %s\n", log_file_path);
        return false;
    }

    return true;
}

// Clean up logging resources
void logging_cleanup(void) {
    if (log_file) {
        fclose(log_file);
        log_file = NULL;
    }
}

// Set minimum log level
void logging_set_level(log_level_t level) {
    min_level = level;
}

// Get current timestamp
static void get_timestamp(char* buffer, size_t size) {
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    strftime(buffer, size, "%Y-%m-%d %H:%M:%S", tm_info);
}

// Write log message
static void write_log(log_level_t level, const char* format, va_list args) {
    if (level < min_level) return;

    char timestamp[20];
    get_timestamp(timestamp, sizeof(timestamp));

    pthread_mutex_lock(&log_mutex);

    // Write to log file
    if (log_file) {
        fprintf(log_file, "%s [%s] ", timestamp, level_strings[level]);
        vfprintf(log_file, format, args);
        fprintf(log_file, "\n");
        fflush(log_file);
    }

    // Write to stderr for ERROR and FATAL
    if (level >= LOG_LEVEL_ERROR) {
        fprintf(stderr, "%s [%s] ", timestamp, level_strings[level]);
        vfprintf(stderr, format, args);
        fprintf(stderr, "\n");
    }

    pthread_mutex_unlock(&log_mutex);
}

// Logging functions
void log_debug(const char* format, ...) {
    va_list args;
    va_start(args, format);
    write_log(LOG_LEVEL_DEBUG, format, args);
    va_end(args);
}

void log_info(const char* format, ...) {
    va_list args;
    va_start(args, format);
    write_log(LOG_LEVEL_INFO, format, args);
    va_end(args);
}

void log_warn(const char* format, ...) {
    va_list args;
    va_start(args, format);
    write_log(LOG_LEVEL_WARN, format, args);
    va_end(args);
}

void log_error(const char* format, ...) {
    va_list args;
    va_start(args, format);
    write_log(LOG_LEVEL_ERROR, format, args);
    va_end(args);
}

void log_fatal(const char* format, ...) {
    va_list args;
    va_start(args, format);
    write_log(LOG_LEVEL_FATAL, format, args);
    va_end(args);
    exit(1);
} 