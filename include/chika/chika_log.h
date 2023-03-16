#ifndef CHIKA_LOG_H__
#define CHIKA_LOG_H__

#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE

#include <string>
#include <functional>
#include <cstdlib>

#include "spdlog/spdlog.h"
#include "spdlog/async.h"

namespace chika {

enum SPDLOG_API LogDst : unsigned char {
  LOG_DST_CONSOLE = 0x01,
  LOG_DST_FILE = 0x02,
  LOG_DST_COLOR = 0x04,
};

class LogFormat;

class SPDLOG_API LogFormatBuilder {
 public:
  using Self = LogFormatBuilder;

  LogFormatBuilder()
  {
    pattern_.reserve(64);
  }

  explicit LogFormatBuilder(std::string pattern)
    : pattern_(std::move(pattern))
  {
  }

  SPDLOG_INLINE Self &AddString(std::string const &str)
  {
    pattern_ += str;
    return *this;
  }

  SPDLOG_INLINE Self &AddString(char const *str)
  {
    pattern_ += str;
    return *this;
  }

#define ADD_PATTERN_STR(str__) \
    pattern_ += str__; \
    return *this

#define DEFINE_ADD_PATTERN_STR(fname__, str__) \
  SPDLOG_INLINE Self &fname__() { \
    ADD_PATTERN_STR(str__); \
  }
  
  DEFINE_ADD_PATTERN_STR(AddContent, "%v")
  DEFINE_ADD_PATTERN_STR(AddThreadId, "%t") 
  DEFINE_ADD_PATTERN_STR(AddProcessId, "%P")
  DEFINE_ADD_PATTERN_STR(AddLoggerName, "%n")
  DEFINE_ADD_PATTERN_STR(AddLogLevel, "%l")
  DEFINE_ADD_PATTERN_STR(AddYear, "%Y")
  DEFINE_ADD_PATTERN_STR(AddMonth, "%m")
  DEFINE_ADD_PATTERN_STR(AddDay, "%d")
  DEFINE_ADD_PATTERN_STR(AddHour, "%H")
  DEFINE_ADD_PATTERN_STR(AddMinute, "%M")
  DEFINE_ADD_PATTERN_STR(AddSecond, "%S") 
  DEFINE_ADD_PATTERN_STR(AddMicroSecond, "%f")
  DEFINE_ADD_PATTERN_STR(AddNanoSecond, "%F")
  DEFINE_ADD_PATTERN_STR(AddPercentageSign, "%%")
  DEFINE_ADD_PATTERN_STR(AddSourceFileName, "%s")
  DEFINE_ADD_PATTERN_STR(AddFullPathSourceFileName, "%g")
  DEFINE_ADD_PATTERN_STR(AddFunctionName, "%!")
  DEFINE_ADD_PATTERN_STR(AddLineNumber, "%#")
  DEFINE_ADD_PATTERN_STR(AddElapsedTimeMs, "%o")
  DEFINE_ADD_PATTERN_STR(AddElapsedTimeUs, "%i")
  DEFINE_ADD_PATTERN_STR(AddElapsedTimeNs, "%u")
  DEFINE_ADD_PATTERN_STR(AddElapsedTimeSec, "%O")
  DEFINE_ADD_PATTERN_STR(AddColorBegin, "%^")
  DEFINE_ADD_PATTERN_STR(AddColorEnd, "%$")
  
  LogFormat Build();
 private:
  friend class LogFormat;
  std::string pattern_;
};

class SPDLOG_API LogFormat {
 public:
  LogFormat() = default;

  explicit LogFormat(LogFormatBuilder &builder)
    : pattern_(std::move(builder.pattern_))
  {
  }

  std::string const &pattern() const noexcept
  {
    return pattern_;
  }
 private:
  std::string pattern_;
};

using LogLevel = spdlog::level::level_enum;
using AsyncLogOverFlowPolicy = spdlog::async_overflow_policy;

struct SPDLOG_API LoggerConfig {
  LoggerConfig();

  /********** ROTATE RELATED ***********/
  std::string rotate_basename;
  size_t rotate_max_file_num;
  size_t rotate_max_file_size;
  // When rotate file sink is created,
  // check destination directory files contains basename
  // If satisfy condition to delete, do it at first.
  bool rotate_check_at_first;

  /********** FLUSH RELATED ***********/
  LogLevel flush_level;
  uint64_t flush_seconds_interval;

  /********** LOG DESTINATION RELATED ***********/
  int log_dst;
  
  /********** ASYNC RELATED ***********/
  int thread_num;
  uint64_t queue_size;
  AsyncLogOverFlowPolicy async_overflow_policy;
  
  /********** LOG CUSTOM FORMAT ***********/
  LogFormat log_format;

  /********** LOG LEVEL ************/
  LogLevel log_level;
  
  using ErrorHandler = spdlog::err_handler;
  ErrorHandler log_error_handler;
};

class SPDLOG_API Logger {
 public:
  explicit Logger(LoggerConfig config);
  
  SPDLOG_INLINE void SetFormat(char const *pattern)
  {
    logger_->set_pattern(pattern);
  }
  
  SPDLOG_INLINE void SetFormat(LogFormat const &format)
  {
    logger_->set_pattern(format.pattern());
  }

  SPDLOG_INLINE void SetLogLevel(LogLevel level)
  {
    logger_->set_level(level);
  }

  SPDLOG_INLINE void Flush()
  {
    logger_->flush();
  }

  SPDLOG_INLINE void SetFlushLevel(LogLevel level)
  {
    logger_->flush_on(level);
  }

  template <typename Rep, typename Period>
  SPDLOG_INLINE void SetFlushInterval(std::chrono::duration<Rep, Period> interval)
  {
    spdlog::flush_every(std::move(interval));
  }

  SPDLOG_INLINE void SetFlushInterval(size_t seconds)
  {
    SetFlushInterval(std::chrono::seconds(seconds));
  }

  SPDLOG_INLINE std::shared_ptr<spdlog::logger> const &logger() const
  {
    return logger_;
  }
 private:
  std::shared_ptr<spdlog::logger> logger_;
  std::shared_ptr<spdlog::details::thread_pool> thread_pool_;
};

SPDLOG_API void setup_logger_config(LoggerConfig &config);
SPDLOG_API Logger &get_logger();

#define CHIKA_LOG(level__, ...) \
  SPDLOG_LOGGER_##level__(chika::get_logger().logger(), __VA_ARGS__)

#define CHIKA_LOG_TRACE(...) CHIKA_LOG(TRACE, __VA_ARGS__)
#define CHIKA_LOG_DEBUG(...) CHIKA_LOG(DEBUG, __VA_ARGS__)
#define CHIKA_LOG_INFO(...) CHIKA_LOG(INFO, __VA_ARGS__)
#define CHIKA_LOG_WARN(...) CHIKA_LOG(WARN, __VA_ARGS__)
#define CHIKA_LOG_ERROR(...) CHIKA_LOG(ERROR, __VA_ARGS__)
#define CHIKA_LOG_FATAL(...) do { \
  CHIKA_LOG(CRITICAL, __VA_ARGS__); \
  get_logger().Flush(); \
  spdlog::shutdown(); \
  abort(); \
} while (0)

} // namespace chika 

#endif
