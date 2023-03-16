#include "chika/chika_log.h"

#include <cstdlib>
#include <type_traits>
#include <cassert>

#include "spdlog/common.h"
//#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/ansicolor_sink.h"
#include "spdlog/sinks/rotating_file_sink.h"
#include "spdlog/logger.h"
#include "chika/split_sink.h"

using namespace chika;
using namespace spdlog;

SPDLOG_INLINE static LogLevel get_default_log_level()
{
  const auto *env = ::getenv("CHIKA_LOG");
  if (!env) return LogLevel::info;
  if (!::strcasecmp(env, "TRACE")) {
    return LogLevel::trace;
  }
  if (!::strcasecmp(env, "DEBUG")) {
    return LogLevel::debug;
  }

  return LogLevel::info;
}

LogFormat LogFormatBuilder::Build()
{
  return LogFormat(*this);
}

LoggerConfig::LoggerConfig()
  : rotate_basename("chika.log")
  , rotate_max_file_num(10)
  , rotate_max_file_size(1 << 22)
  , rotate_check_at_first()
  , flush_level(LogLevel::critical)
  , flush_seconds_interval(3)
  , log_dst(LOG_DST_CONSOLE)
  , thread_num(1)
  , queue_size(8192 * 2)
  , async_overflow_policy(spdlog::async_overflow_policy::overrun_oldest)
  , log_format(LogFormatBuilder{}
    .AddYear().AddString("/").AddMonth().AddString("/").AddDay().AddString("-")
    .AddHour().AddString(":").AddMinute().AddString(":").AddSecond().AddString(".").AddMicroSecond().AddString(" ")
    .AddThreadId().AddString(" ")
    .AddFunctionName().AddString(" [")
    .AddColorBegin().AddLogLevel().AddColorEnd().AddString("] ")
    .AddContent().AddString(" - ")
    .AddSourceFileName().AddString(":").AddLineNumber()
    .Build())
  , log_level(get_default_log_level())
  , log_error_handler([](std::string const &msg) {
      fprintf(stderr, "There are some fatal error happened in chika log\n");
      fprintf(stderr, "Now, abort entire program(Default handling)\n");
      fflush(stderr);
      fflush(stdout);
      abort();
      })
{
}

Logger::Logger(LoggerConfig config)
{
  std::vector<sink_ptr> sinks;
  if (config.log_dst & LOG_DST_CONSOLE) {
    if (config.log_dst & LOG_DST_COLOR) {
      //sinks.emplace_back(std::make_shared<sinks::stdout_color_sink_mt>());
      auto color_sink = std::make_shared<sinks::ansicolor_sink<spdlog::details::console_mutex>>(stdout, stderr, LogLevel::err);

      constexpr LogLevel s_level_list[] {
        LogLevel::trace,
        LogLevel::debug,
        LogLevel::info,
        LogLevel::warn,
        LogLevel::err,
        LogLevel::critical,
      };

      const string_view_t s_level_color[] = {
        color_sink->cyan,
        color_sink->blue,
        color_sink->green,
        color_sink->yellow,
        color_sink->red,
        color_sink->red,
      };
      
      auto const level_num = sizeof(s_level_list) / sizeof(s_level_list[0]);
      assert(s_level_num == sizeof(s_level_color) / sizeof(s_level_color[0]));
      for (int i = 0; i < level_num; ++i) {
        color_sink->set_color(s_level_list[i], s_level_color[i]);
      }

      sinks.emplace_back(std::move(color_sink));
    } else {
      auto split_sink = std::make_shared<ConsoleSplitSink_mt>();
      sinks.emplace_back(std::move(split_sink));
    }
  }
  if (config.log_dst & LOG_DST_FILE) {
    sinks.emplace_back(std::make_shared<sinks::rotating_file_sink_mt>(
          config.rotate_basename,
          config.rotate_max_file_size,
          config.rotate_max_file_num,
          config.rotate_check_at_first));
  }
  
  // There ctor of async_logger accept std::weak_ptr.
  // We must eusure the lifetime
  thread_pool_ = std::make_shared<details::thread_pool>(config.queue_size, config.thread_num);
  logger_ = std::make_shared<async_logger>("ChikaAsyncLog", sinks.begin(), sinks.end(), thread_pool_, config.async_overflow_policy);

  SetFlushInterval(config.flush_seconds_interval);
  SetFlushLevel(config.flush_level);
  SetFormat(config.log_format);
  SetLogLevel(config.log_level);

  logger_->set_error_handler(std::move(config.log_error_handler));
  register_logger(logger_);
}

static LoggerConfig *p_config = nullptr;

void chika::setup_logger_config(LoggerConfig &config)
{
  p_config = &config;
}

Logger &chika::get_logger()
{
  static Logger logger(std::move(p_config ? *p_config : LoggerConfig{}));
  return logger;
}
