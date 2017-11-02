#ifndef TALK_LOGGER_H
#define TALK_LOGGER_H

#include <memory>
#include <string>
#include "spdlog/spdlog.h"

extern std::shared_ptr<spdlog::logger> LOG;
void console_log_init();
void file_log_init(const std::string& filename);

#endif // TALK_LOGGER_H
