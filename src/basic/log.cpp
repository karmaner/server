#include "basic/log.h"

#include <ctime>
#include <ostream>
#include <sstream>
#include <string>
#include <functional>

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
  DateTimeFormatItem(const std::string& fmt = "%Y-%m-%d %H:%M:%S")
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
  ThreadIdFormatItem(const std::string& str = "") {}
  void format(std::ostream &os, LogEvent::ptr event) override {
    os << event->getThreadId();
  }
};

class ThreadNameFormatItem : public LogFormat::FormatItem {
public:
  ThreadNameFormatItem(const std::string& str = "") {}
  void format(std::ostream &os, LogEvent::ptr event) override {
    os << event->getThreadName();
  }
};

class FiberIdFormatItem : public LogFormat::FormatItem {
public:
  FiberIdFormatItem(const std::string& str = "") {}
  void format(std::ostream &os, LogEvent::ptr event) override {
    os << event->getFiberId();
  }
};

class MessageFormatItem : public LogFormat::FormatItem {
public:
  MessageFormatItem(const std::string& str = "") {}
  void format(std::ostream &os, LogEvent::ptr event) override {
    os << event->getContent();
  }
};

class NewLineFormatItem : public LogFormat::FormatItem {
public:
  NewLineFormatItem(const std::string& str = "") {}
  void format(std::ostream &os, LogEvent::ptr event) override {
    os << "\n";
  }
};

class TabFormatItem : public LogFormat::FormatItem {
public:
  TabFormatItem(const std::string& str = "") {}
  void format(std::ostream &os, LogEvent::ptr event) override {
    os << "\t";
  }
};

class LogNameFormatItem : public LogFormat::FormatItem {
public:
  LogNameFormatItem(const std::string& str = "") {}
  void format(std::ostream& os, LogEvent::ptr event) override {
    os << event->getLogName();
  }
};

class StringFormatItem : public LogFormat::FormatItem {
public:
  StringFormatItem(const std::string& str): m_string(str) {}
  void format(std::ostream& os, LogEvent::ptr event) override {
    os << m_string;
  }
private:
  std::string m_string;
};

// TODO: 需要解析的格式未
// "%d{%Y-%m-%d %H:%M:%S} [%p] [%N:%t] [F:%F] %c %f:%l >> %m%n"
void LogFormat::init() {

  if (m_pattern.empty()) {
    m_error = true;
    return;
  }

  struct PatternToken {
    std::string type;
    std::string fmt;
    std::string text;
  };

  std::vector<PatternToken> tokens;
  std::string_view pattern(m_pattern);
  std::string tmp;
  tmp.reserve(pattern.size());

  for (size_t i = 0; i < m_pattern.size(); ++i) {
    char ch = pattern[i];
    if (ch != '%') {
      tmp.push_back(ch);
      continue;
    }

    if (!tmp.empty()) {
      tokens.push_back({"", "", std::move(tmp)});
      tmp.clear();
    }

    // 检查边界
    if (i + 1 >= m_pattern.size()) {
      m_error = true;
      return;
    }

    char type = m_pattern[++i];
    std::string fmt;

    // 解析 %d{...} 格式
    if (i + 1 < m_pattern.size() && m_pattern[i + 1] == '{') {
      size_t start = i + 2;
      size_t end = pattern.find('}', start);
      if (end == std::string::npos) {
        m_error = true;
        break;
      }

      fmt = std::string(pattern.substr(start, end - start));
      i = end; // 跳过 '}'
    }

    tokens.push_back({std::string(1, type), fmt, ""});
  }

  if (!tmp.empty()) tokens.push_back({"", "", std::move(tmp)});

    static const std::unordered_map<std::string, std::function<FormatItem::ptr(const std::string&)>> s_items = {
      {"m", [](auto& f){ return std::make_shared<MessageFormatItem>(); }},
      {"p", [](auto& f){ return std::make_shared<LevelFormatItem>(); }},
      {"c", [](auto& f){ return std::make_shared<LogNameFormatItem>(); }},
      {"t", [](auto& f){ return std::make_shared<ThreadIdFormatItem>(); }},
      {"N", [](auto& f){ return std::make_shared<ThreadNameFormatItem>(); }},
      {"d", [](auto& f){ return std::make_shared<DateTimeFormatItem>(f.empty() ? "%Y-%m-%d %H:%M:%S" : f); }},
      {"f", [](auto& f){ return std::make_shared<FileNameFormatItem>(); }},
      {"l", [](auto& f){ return std::make_shared<LineFormatItem>(); }},
      {"F", [](auto& f){ return std::make_shared<FiberIdFormatItem>(); }},
      {"n", [](auto& f){ return std::make_shared<NewLineFormatItem>(); }},
      {"T", [](auto& f){ return std::make_shared<TabFormatItem>(); }},
    };

  for (auto& tk : tokens) {
    if (!tk.type.empty()) {
      auto it = s_items.find(tk.type);
      if (it != s_items.end()) {
        m_items.push_back(it->second(tk.fmt));
      } else {
        m_items.push_back(std::make_shared<StringFormatItem>("<<error %" + tk.type + ">>"));
        m_error = true;
      }
    } else {
      m_items.push_back(std::make_shared<StringFormatItem>(tk.text));
    }
  }
}

std::ostream& LogFormat::format(std::ostream& os, LogEvent::ptr event) {
  for (auto& i : m_items) {
    i->format(os, event);
  }
  return os;
}

std::string LogFormat::format(LogEvent::ptr event) {
  std::stringstream ss;
  for (auto& i : m_items) {
    i->format(ss, event);
  }
  return ss.str();
}