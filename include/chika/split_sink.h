#ifndef CHIKA_LOG_SPLIT_SINK_H__
#define CHIKA_LOG_SPLIT_SINK_H__

#include "spdlog/sinks/base_sink.h"

#include "spdlog/details/null_mutex.h"
#include <mutex>

namespace chika {

template <typename Mutex>
class ConsoleSplitSink : public spdlog::sinks::base_sink<Mutex> {
 public:
  using Base = spdlog::sinks::base_sink<Mutex>;
 protected:
  void sink_it_(spdlog::details::log_msg const &msg) override
  {
    spdlog::memory_buf_t formatted;
    Base::formatter_->format(msg, formatted);
    FILE *log_dst = stdout;
    if (msg.level >= spdlog::level::err) {
      log_dst = stderr;
    }
    fmt::print(log_dst, fmt::to_string(formatted));
  }

  void flush_() override
  {
    fflush(stderr);
    fflush(stdout);
  }
};

using ConsoleSplitSink_mt = ConsoleSplitSink<std::mutex>;
using ConsoleSplitSink_st = ConsoleSplitSink<spdlog::details::null_mutex>;

} // namespace chika

#endif
