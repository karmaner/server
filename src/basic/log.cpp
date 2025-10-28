#include "basic/log.h"

#include <ctime>

/*========================= LogLevel ========================*/

LogLevel::Level LogLevel::from_string(std::string str) {
#define XX(level, val)                                                         \
  if (str == #val)                                                             \
    return LogLevel::level;
  XX(TRACE, trace);
  XX(DEBUG, debug);
  XX(INFO, info);
  XX(WARN, warn);
  XX(ERROR, error);
  XX(FATAL, fatal);

  XX(TRACE, TRACE);
  XX(DEBUG, DEBUG);
  XX(INFO, INFO);
  XX(WARN, WARN);
  XX(ERROR, ERROR);
  XX(FATAL, FATAL);
#undef XX
  return LogLevel::UNKNOWN;
};

const char *LogLevel::to_string(LogLevel::Level level) {
  switch (level) {
#define XX(lv)                                                                 \
  case lv:                                                                     \
    return #lv;
    XX(TRACE)
    XX(DEBUG)
    XX(INFO)
    XX(WARN)
    XX(ERROR)
    XX(FATAL)
#undef XX
  default:
    return "UNKNOWN";
  }
}

/*========================= LogEvent ========================*/

LogEvent::LogEvent(std::shared_ptr<Log> log, 
                   LogLevel::Level level,
                   const char* file,
                   int32_t line,
                   uint32_t elapse,
                   uint32_t thread_id, 
                   uint32_t fiber_id,
                   const std::string& thread_name)
  : m_log(log),
    m_level(level), 
    m_file(file),
    m_line(line),
    m_elapse(elapse),
    m_threadId(thread_id),
    m_fiberId(fiber_id),
    m_threadName(thread_name) {}

  const std::string& LogEvent::getLogName() const {
    return m_log->getName();
  }

void LogEvent::log() {
  if (m_level >= m_log->getLevel()) {
    m_log->log(shared_from_this());
  }
}

/*========================= LogEventWrap ========================*/

LogEventWrap::LogEventWrap(LogEvent::ptr event) : m_event(event){};

LogEventWrap::~LogEventWrap() {
  if (m_event) {
    m_event->log();
  }
}

/*========================= LogFormat ========================*/

LogFormat::LogFormat(const std::string& pattern) {
  if (pattern.empty()) {
    m_pattern = "%d{%Y-%m-%d %H:%M:%S} [%p] [%N:%t] [F:%F] %c %f:%l >> %m%n";
  }
  m_pattern = pattern;
  init();
}

class FileNameFormatItem : public LogFormat::FormatItem {
public:
  void format(std::ostream &os, LogEvent::ptr event) override {
    os << event->getFile();
  }
};

class LineFormatItem : public LogFormat::FormatItem {
public:
  void format(std::ostream &os, LogEvent::ptr event) override {
    os << event->getLine();
  }
};

class LevelFormatItem : public LogFormat::FormatItem {
public:
  void format(std::ostream &os, LogEvent::ptr event) override {
    os << LogLevel::to_string(event->getLevel());
  }
};

class ElapseFormatItem : public LogFormat::FormatItem {
public:
  void format(std::ostream &os, LogEvent::ptr event) override {
    os << event->getElapse();
  }
};

class DateTimeFormatItem : public LogFormat::FormatItem {
public:
  DateTimeFormatItem(const std::string &fmt = "%Y-%m-%d %H:%M:%S")
    : m_fmt(fmt) {}
  void format(std::ostream &os, LogEvent::ptr event) override {
    std::time_t t = event->getTime();
    struct tm tm;
    localtime_r(&t, &tm);
    char buf[64];
    strftime(buf, sizeof(buf), m_fmt.c_str(), &tm);
    os << buf;
  }

private:
  std::string m_fmt;
};

class ThreadIdFormatItem : public LogFormat::FormatItem {
public:
  void format(std::ostream &os, LogEvent::ptr event) override {
    os << event->getThreadId();
  }
};

class ThreadNameFormatItem : public LogFormat::FormatItem {
public:
  void format(std::ostream &os, LogEvent::ptr event) override {
    os << event->getThreadName();
  }
};

class FiberIdFormatItem : public LogFormat::FormatItem {
public:
  void format(std::ostream &os, LogEvent::ptr event) override {
    os << event->getFiberId();
  }
};

class MessageFormatItem : public LogFormat::FormatItem {
public:
  void format(std::ostream &os, LogEvent::ptr event) override {
    os << event->getContent();
  }
};

class NewLineFormatItem : public LogFormat::FormatItem {
public:
  void format(std::ostream &os, LogEvent::ptr event) override {
    os << "\n";
  }
};

class TabFormatItem : public LogFormat::FormatItem {
public:
  void format(std::ostream &os, LogEvent::ptr event) override {
    os << "\t";
  }
};

class LogNameFormatItem : public LogFormat::FormatItem {
public:
  void format(std::ostream &os, LogEvent::ptr event) override {
    os << event->getLogName();
  }
};

const std::unordered_map<std::string, std::function<LogFormat::FormatItem::ptr()>>& LogFormat::getFormatMaps() {
  static const std::unordered_map<std::string, std::function<LogFormat::FormatItem::ptr()>> s_format_items = {
  {"m", [](){ return LogFormat::FormatItem::ptr(new MessageFormatItem()); }},    // 消息内容
  {"p", [](){ return LogFormat::FormatItem::ptr(new LevelFormatItem()); }},      // 日志级别
  {"r", [](){ return LogFormat::FormatItem::ptr(new ElapseFormatItem()); }},     // 累计耗时（毫秒）
  {"c", [](){ return LogFormat::FormatItem::ptr(new LogNameFormatItem()); }},    // 日志器名称
  {"t", [](){ return LogFormat::FormatItem::ptr(new ThreadIdFormatItem()); }},   // 线程ID
  {"F", [](){ return LogFormat::FormatItem::ptr(new FiberIdFormatItem()); }},    // 协程ID
  {"n", [](){ return LogFormat::FormatItem::ptr(new NewLineFormatItem()); }},    // 换行符
  {"d", [](){ return LogFormat::FormatItem::ptr(new DateTimeFormatItem()); }},   // 日期时间
  {"f", [](){ return LogFormat::FormatItem::ptr(new FileNameFormatItem()); }},   // 文件名
  {"l", [](){ return LogFormat::FormatItem::ptr(new LineFormatItem()); }},       // 行号
  {"N", [](){ return LogFormat::FormatItem::ptr(new ThreadNameFormatItem()); }},// 线程名称
  {"T", [](){ return LogFormat::FormatItem::ptr(new TabFormatItem()); }}        // 制表符
};

  return s_format_items;
}

// TODO: 需要解析的格式未
// "%d{%Y-%m-%d %H:%M:%S} [%p] [%N:%t] [F:%F] %c %f:%l >> %m%n"
void LogFormat::init() {
  if (m_pattern.empty()) {
    m_error = true;
    return;
  }

  std::string tmp;
  // 存储解析结果 （type, format, string)
  std::vector<std::tuple<std::string, std::string, std::string> > patterns;
  for (size_t i = 0; i < m_pattern.size(); ++i) {
    if (m_pattern[i] != '%') {
      tmp += m_pattern[i];
      continue;
    }

    if (!tmp.empty()) {
      patterns.push_back(std::make_tuple("", "", tmp));
      tmp.clear();
    }

    char c = m_pattern[i];
    std::string fmt;

    if (i + 1 < m_pattern.size() && m_pattern[i + 1] == '{') {
      size_t j = i + 2;
      while (j < m_pattern.size() && m_pattern[j] != '}') {
        fmt += m_pattern[j];
        ++j;
      }
      if (j < m_pattern.size()) {
        i = j;  // 跳过 '}'
      }
    }
  }

}