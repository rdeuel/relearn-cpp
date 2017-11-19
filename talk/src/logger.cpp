#include <string>
#include "logger.h"

using namespace std;

shared_ptr<spdlog::logger> LOG;

void console_log_init() {
    LOG = spdlog::stdout_logger_mt("talk");
    LOG->set_level(spdlog::level::debug);
    LOG->set_pattern("%T | thread-%t | %L | %v");
}

void file_log_init(const string& filename) {
    LOG = spdlog::basic_logger_mt("talk", filename);
    LOG->set_level(spdlog::level::debug);
    LOG->set_pattern("%T | thread-%t | %L | %v");
}
