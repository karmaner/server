#include "basic/log.h"

#include <pthread.h>
#include <yaml-cpp/yaml.h>

#include <cstdarg>
#include <ctime>
#include <functional>
#include <iostream>
#include <memory>
#include <ostream>
#include <sstream>
#include <string>

#include "basic/config.h"
#include "basic/mutex.h"

namespace Basic {

/*========================= LogLevel ========================*/

LogLevel::Level LogLevel::from_string(const std::string& str) {
#define XX(level, val) \
  if (str == #val) return LogLevel::level;
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

const char* LogLevel::to_string(LogLevel::Level level) {
  switch (level) {
#define XX(lv) \
  case lv:     \
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

LogEvent::LogEvent(std::shared_ptr<Log> log, LogLevel::Level level, const char* file, int32_t line,
                   uint64_t time, uint64_t elapse, uint32_t thread_id, uint32_t fiber_id,
                   const std::string& thread_name)
    : m_log(log),
      m_level(level),
      m_file(file),
      m_line(line),
      m_time(time),
      m_elapse(elapse),
      m_threadId(thread_id),
      m_fiberId(fiber_id),
      m_threadName(thread_name) {}

const std::string& LogEvent::getLogName() const {
  return m_log->getName();
}

void LogEvent::format(const char* fmt, ...) {
  va_list al;
  va_start(al, fmt);
  format(fmt, al);
  va_end(al);
}

void LogEvent::format(const char* fmt, va_list al) {
  char* buf = nullptr;
  int   len = vasprintf(&buf, fmt, al);
  if (len != -1) {
    m_ss << std::string(buf, len);
    free(buf);
  }
}

/*========================= LogEventWrap ========================*/

LogEventWrap::LogEventWrap(LogEvent::ptr event) : m_event(event){};

LogEventWrap::~LogEventWrap() {
  if (m_event) { m_event->getLog()->log(m_event->getLevel(), m_event); }
}

/*========================= LogFormat ========================*/

LogFormat::LogFormat(const std::string& pattern) {
  if (pattern.empty()) {
    m_pattern = "%d{%Y-%m-%d %H:%M:%S}%T%t%T%N%T%F%T[%p]%T[%c]%T%f:%l%T >> %m%n";
  }
  m_pattern = pattern;
  init();
}

class FileNameFormatItem : public LogFormat::FormatItem {
public:
  void format(std::ostream& os, LogEvent::ptr event) override { os << event->getFile(); }
};

class LineFormatItem : public LogFormat::FormatItem {
public:
  void format(std::ostream& os, LogEvent::ptr event) override { os << event->getLine(); }
};

class LevelFormatItem : public LogFormat::FormatItem {
public:
  void format(std::ostream& os, LogEvent::ptr event) override {
    os << LogLevel::to_string(event->getLevel());
  }
};

class ElapseFormatItem : public LogFormat::FormatItem {
public:
  void format(std::ostream& os, LogEvent::ptr event) override { os << event->getElapse(); }
};

class DateTimeFormatItem : public LogFormat::FormatItem {
public:
  DateTimeFormatItem(const std::string& fmt = "%Y-%m-%d %H:%M:%S") : m_fmt(fmt) {}
  void format(std::ostream& os, LogEvent::ptr event) override {
    std::time_t t = event->getTime();
    struct tm   tm;
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
  void format(std::ostream& os, LogEvent::ptr event) override { os << event->getThreadId(); }
};

class ThreadNameFormatItem : public LogFormat::FormatItem {
public:
  ThreadNameFormatItem(const std::string& str = "") {}
  void format(std::ostream& os, LogEvent::ptr event) override { os << event->getThreadName(); }
};

class FiberIdFormatItem : public LogFormat::FormatItem {
public:
  FiberIdFormatItem(const std::string& str = "") {}
  void format(std::ostream& os, LogEvent::ptr event) override { os << event->getFiberId(); }
};

class MessageFormatItem : public LogFormat::FormatItem {
public:
  MessageFormatItem(const std::string& str = "") {}
  void format(std::ostream& os, LogEvent::ptr event) override { os << event->getContent(); }
};

class NewLineFormatItem : public LogFormat::FormatItem {
public:
  NewLineFormatItem(const std::string& str = "") {}
  void format(std::ostream& os, LogEvent::ptr event) override { os << "\n"; }
};

class TabFormatItem : public LogFormat::FormatItem {
public:
  TabFormatItem(const std::string& str = "") {}
  void format(std::ostream& os, LogEvent::ptr event) override { os << "\t"; }
};

class LogNameFormatItem : public LogFormat::FormatItem {
public:
  LogNameFormatItem(const std::string& str = "") {}
  void format(std::ostream& os, LogEvent::ptr event) override { os << event->getLogName(); }
};

class StringFormatItem : public LogFormat::FormatItem {
public:
  StringFormatItem(const std::string& str) : m_string(str) {}
  void format(std::ostream& os, LogEvent::ptr event) override { os << m_string; }

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
  std::string_view          pattern(m_pattern);
  std::string               tmp;
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

    char        type = m_pattern[++i];
    std::string fmt;

    // 解析 %d{...} 格式
    if (i + 1 < m_pattern.size() && m_pattern[i + 1] == '{') {
      size_t start = i + 2;
      size_t end   = pattern.find('}', start);
      if (end == std::string::npos) {
        m_error = true;
        break;
      }

      fmt = std::string(pattern.substr(start, end - start));
      i   = end;  // 跳过 '}'
    }

    tokens.push_back({std::string(1, type), fmt, ""});
  }

  if (!tmp.empty()) tokens.push_back({"", "", std::move(tmp)});

  static const std::unordered_map<std::string, std::function<FormatItem::ptr(const std::string&)>>
      s_items = {
          {"m", [](auto& f) { return std::make_shared<MessageFormatItem>(); }},
          {"p", [](auto& f) { return std::make_shared<LevelFormatItem>(); }},
          {"c", [](auto& f) { return std::make_shared<LogNameFormatItem>(); }},
          {"t", [](auto& f) { return std::make_shared<ThreadIdFormatItem>(); }},
          {"N", [](auto& f) { return std::make_shared<ThreadNameFormatItem>(); }},
          {"d",
           [](auto& f) {
             return std::make_shared<DateTimeFormatItem>(f.empty() ? "%Y-%m-%d %H:%M:%S" : f);
           }},
          {"f", [](auto& f) { return std::make_shared<FileNameFormatItem>(); }},
          {"l", [](auto& f) { return std::make_shared<LineFormatItem>(); }},
          {"F", [](auto& f) { return std::make_shared<FiberIdFormatItem>(); }},
          {"n", [](auto& f) { return std::make_shared<NewLineFormatItem>(); }},
          {"T", [](auto& f) { return std::make_shared<TabFormatItem>(); }},
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

/*========================= LogAppender ========================*/

LogFormat::ptr LogAppender::getFormatter() {
  LockType lock(m_lock);
  return m_formatter;
}

void LogAppender::setFormatter(LogFormat::ptr val) {
  LockType lock(m_lock);
  m_formatter = val;
  if (m_formatter) {
    m_hasFormatter = true;
  } else {
    m_hasFormatter = false;
  }
}

/*========================= Log ========================*/

Log::Log(const std::string& name) : m_name(name), m_level(LogLevel::DEBUG) {
  m_formatter.reset(new LogFormat("%d{%Y-%m-%d %H:%M:%S}%T%t%T%N%T%F%T[%p]%T[%c]%T%f:%l%T%m%n"));
}

void Log::addAppender(LogAppender::ptr appender) {
  LockType::Lock lock(m_lock);
  if (!appender->getFormatter()) {
    LockType lock2(appender->m_lock);
    appender->m_formatter = m_formatter;
  }
  m_appenders.push_back(appender);
}

void Log::delAppender(LogAppender::ptr appender) {
  LockType::Lock lock(m_lock);
  for (auto it = m_appenders.begin(); it != m_appenders.end(); ++it) {
    if (*it == appender) {
      LockType lock2(appender->m_lock);
      m_appenders.erase(it);
      break;
    }
  }
}

void Log::clearAppender() {
  LockType::Lock lock(m_lock);
  m_appenders.clear();
}

void Log::log(LogLevel::Level level, LogEvent::ptr e) {
  if (level >= m_level) {
    auto           self = shared_from_this();
    LockType::Lock lock(m_lock);
    if (!m_appenders.empty()) {
      for (auto& i : m_appenders) {
        i->log(self, level, e);
      }
    } else if (m_root) {
      m_root->log(level, e);
    }
  }
}

void Log::trace(LogEvent::ptr event) {
  log(LogLevel::TRACE, event);
}

void Log::debug(LogEvent::ptr event) {
  log(LogLevel::DEBUG, event);
}

void Log::info(LogEvent::ptr event) {
  log(LogLevel::INFO, event);
}

void Log::warn(LogEvent::ptr event) {
  log(LogLevel::WARN, event);
}

void Log::error(LogEvent::ptr event) {
  log(LogLevel::ERROR, event);
}

void Log::fatal(LogEvent::ptr event) {
  log(LogLevel::FATAL, event);
}

LogFormat::ptr Log::getFormat() {
  return m_formatter;
}

void Log::setFormat(LogFormat::ptr format) {
  LockType::Lock lock(m_lock);
  m_formatter = format;
  for (auto& i : m_appenders) {
    LockType lock2(i->m_lock);
    if (!i->m_hasFormatter) { i->setFormatter(format); }
  }
}

void Log::setFormat(const std::string& format) {
  LogFormat::ptr new_val(new LogFormat(format));
  if (new_val->isError()) {
    std::cout << "Log setFormat name=" << m_name << " value=" << format << " invalid formatter"
              << std::endl;
    return;
  }
  setFormat(new_val);
}

std::string Log::toYamlString() {
  LockType::Lock lock(m_lock);
  YAML::Node     node;
  node["name"] = m_name;
  if (m_level != LogLevel::UNKNOWN) { node["level"] = LogLevel::to_string(m_level); }
  if (m_formatter) { node["format"] = m_formatter->getPattern(); }

  for (auto& i : m_appenders) {
    node["appenders"].push_back(YAML::Load(i->toYamlString()));
  }
  std::stringstream ss;
  ss << node;
  return ss.str();
}

/*========================= LogManager ========================*/

LogManager::LogManager() {
  m_root.reset(new Log);
  m_root->addAppender(LogAppender::ptr(new StdoutAppender));

  m_logs[m_root->m_name] = m_root;

  init();
}

Log::ptr LogManager::getRoot() {
  if (!m_root) { m_root = std::make_shared<Log>("root"); }
  return m_root;
}

Log::ptr LogManager::getLog(const std::string& name) {
  LockType::Lock lock(m_lock);
  auto           it = m_logs.find(name);
  if (it != m_logs.end()) { return it->second; }

  Log::ptr log(new Log(name));
  log->m_root  = m_root;
  m_logs[name] = log;
  return log;
}

void LogManager::init() {}

std::string LogManager::toYamlString() {
  YAML::Node node;
  for (auto& i : m_logs) {
    node.push_back(YAML::Load(i.second->toYamlString()));
  }
  std::stringstream ss;
  ss << node;
  return ss.str();
}

/*========================= StdoutAppender ========================*/

void StdoutAppender::log(std::shared_ptr<Log> log, LogLevel::Level level, LogEvent::ptr event) {
  LockType lock(m_lock);
  if (level >= m_level) { m_formatter->format(std::cout, event); }
}

std::string StdoutAppender::toYamlString() {
  LockType   lock(m_lock);
  YAML::Node node;
  node["type"] = "StdoutAppender";
  if (m_level != LogLevel::UNKNOWN) { node["level"] = LogLevel::to_string(m_level); }
  if (m_hasFormatter && m_formatter) { node["format"] = m_formatter->getPattern(); }
  std::stringstream ss;
  ss << node;
  return ss.str();
}

/*========================= FileAppender ========================*/

FileAppender::FileAppender(const std::string& name) : m_filename(name) {
  m_lastTime = 0;
  reopen();
}

bool FileAppender::reopen() {
  if (m_filestream) { m_filestream.close(); }

  m_filestream.open(m_filename, std::ios::app);
  if (!m_filestream.is_open()) { return false; }

  return true;
}

void FileAppender::log(std::shared_ptr<Log> log, LogLevel::Level level, LogEvent::ptr event) {
  if (level >= m_level) {
    uint64_t now = event->getTime();
    if (now >= (m_lastTime + 3)) {
      reopen();
      m_lastTime = now;
    }
    LockType lock(m_lock);
    if (!m_formatter->format(m_filestream, event)) {
      std::cout << "FileAppender::log error" << std::endl;
    };
  }
}

std::string FileAppender::toYamlString() {
  LockType   lock(m_lock);
  YAML::Node node;
  node["type"] = "FileAppender";
  if (m_level != LogLevel::UNKNOWN) { node["level"] = LogLevel::to_string(m_level); }
  if (m_hasFormatter && m_formatter) { node["format"] = m_formatter->getPattern(); }
  node["file"] = m_filename;

  std::stringstream ss;
  ss << node;
  return ss.str();
}

/*========================= NetAppender ========================*/

/*========================== 日志文件读取 ========================*/

template <>
class LexicalCast<std::string, LogDefine> {
public:
  LogDefine operator()(const std::string& v) {
    YAML::Node node = YAML::Load(v);
    LogDefine  ld;
    if (!node["name"].IsDefined()) {
      std::cout << "log config file error: name is null" << node << std::endl;
      throw std::logic_error("log config file name is null");
    }
    ld.name = node["name"].as<std::string>();
    ld.level =
        LogLevel::from_string(node["level"].IsDefined() ? node["level"].as<std::string>() : "");
    if (node["format"].IsDefined()) { ld.formatter = node["format"].as<std::string>(); }
    if (node["appenders"].IsDefined()) {
      for (size_t i = 0; i < node["appenders"].size(); ++i) {
        auto a = node["appenders"][i];
        if (!a["type"].IsDefined()) {
          std::cout << "log config file error : aappender type is null" << a << std::endl;
          continue;
          ;
        }
        std::string       type = a["type"].as<std::string>();
        LogAppenderDefine lad;
        if (type == "StdoutAppender") {
          lad.type = 1;
        } else if (type == "FileAppender") {
          lad.type = 2;
          if (!a["file"].IsDefined()) {
            std::cout << "log config file error: fileappender file is null" << a << std::endl;
            continue;
          }
          lad.file = a["file"].as<std::string>();
        } else {
          std::cout << "log config file error: appender type is invalid, " << a << std::endl;
          continue;
        }

        lad.level =
            LogLevel::from_string(a["level"].IsDefined() ? a["level"].as<std::string>() : "");
        lad.formatter = a["format"].IsDefined() ? a["format"].as<std::string>() : "";

        ld.appenders.push_back(lad);
      }
    }
    return ld;
  }
};

template <>
class LexicalCast<LogDefine, std::string> {
public:
  std::string operator()(const LogDefine& i) {
    YAML::Node n;
    n["name"] = i.name;
    if (i.level != LogLevel::UNKNOWN) { n["level"] = LogLevel::to_string(i.level); }
    if (!i.formatter.empty()) { n["format"] = i.formatter; }

    for (auto& a : i.appenders) {
      YAML::Node na;
      if (a.type == 1) {
        na["type"] = "StdoutAppender";
      } else if (a.type == 2) {
        na["type"] = "FileAppender";
        na["file"] = a.file;
      }
      if (a.level != LogLevel::UNKNOWN) { na["level"] = LogLevel::to_string(a.level); }

      if (!a.formatter.empty()) { na["format"] = a.formatter; }

      n["appenders"].push_back(na);
    }
    std::stringstream ss;
    ss << n;
    return ss.str();
  }
};

ConfigVar<std::set<LogDefine>>::ptr g_log_defines =
    Config::Lookup("logs", std::set<LogDefine>(), "日志合集");

struct LogIniter {
  LogIniter() {
    g_log_defines->addListener(
        [](const std::set<LogDefine>& old_value, const std::set<LogDefine>& new_value) {
          LOG_INFO("on_log_conf_changed");

          for (auto& i : new_value) {
            auto     it = old_value.find(i);
            Log::ptr log;
            if (it == old_value.end()) {
              // 新增log
              log = LOG_NAME(i.name);
            } else {
              if (!(i == *it)) {
                // 修改的log
                log = LOG_NAME(i.name);
              } else {
                continue;
              }
            }
            log->setLevel(i.level);
            if (!i.formatter.empty()) { log->setFormat(i.formatter); }

            log->clearAppender();
            for (auto& a : i.appenders) {
              LogAppender::ptr ap;
              if (a.type == 1) {
                ap.reset(new StdoutAppender);
              } else if (a.type == 2) {
                ap.reset(new FileAppender(a.file));
              } else {
                continue;
              }
              ap->setLevel(a.level);
              if (!a.formatter.empty()) {
                LogFormat::ptr fmt(new LogFormat(a.formatter));
                if (!fmt->isError()) {
                  ap->setFormatter(fmt);
                } else {
                  std::cout << "log.name=" << i.name << " appender type=" << a.type
                            << " formatter=" << a.formatter << " is invalid" << std::endl;
                }
              }
              log->addAppender(ap);
            }
          }

          for (auto& i : old_value) {
            auto it = new_value.find(i);
            if (it == new_value.end()) {
              // 删除log
              auto log = LOG_NAME(i.name);
              log->setLevel((LogLevel::Level)0);
              log->clearAppender();
            }
          }
        });
  }
};

static LogIniter __log_init;

}  // namespace Basic