#pragma once

#include <stdint.h>

#include <fstream>
#include <memory>
#include <ostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>
#include <list>

#include "basic/singletion.h"

class Log;

/**
 * @brief 日志等级类
 */
class LogLevel {
public:
  /**
   * @brief 日志等级枚举
   */
  enum Level {
    UNKNOWN = 0, ///< 未知等级
    TRACE,       ///< 跟踪信息
    DEBUG,       ///< 调试信息
    INFO,        ///< 一般信息
    WARN,        ///< 警告信息
    ERROR,       ///< 错误信息
    FATAL        ///< 致命错误
  };

  static Level from_string(std::string str);
  static const char* to_string(Level level);
};

/**
 * @brief 日志事件
 */
class LogEvent : public std::enable_shared_from_this<LogEvent> {
public:
  typedef std::shared_ptr<LogEvent> ptr;

  LogEvent(
    std::shared_ptr<Log> log,
    LogLevel::Level level,
    const char* file,
    int32_t line,
    uint32_t elapse,
    uint32_t thread_id,
    uint32_t fiber_id,
    const std::string& thread_name);

  const std::string& getLogName() const;
  const uint64_t getTime() const { return m_time; }
  const uint64_t getElapse() const { return m_elapse; }
  const std::string& getThreadName() const { return m_threadName; }
  const uint32_t getThreadId() const { return m_threadId; }
  const uint64_t getFiberId() const { return m_fiberId; }
  const LogLevel::Level getLevel() const { return m_level; }
  const char* getFile() const { return m_file; }
  const int getLine() const { return m_line; }
  const std::stringstream& getSS() const { return m_ss; }
  std::string getContent() const { return m_ss.str(); }

  void log();
  void format(const char* fmt, ...);
  void format(const char* fmt, va_list al);

private:
  uint64_t m_time = 0;                             ///< 时间戳
  uint64_t m_elapse = 0;                           ///< 程序运行时长 ms
  uint32_t m_threadId = 0;                         ///< 线程id
  std::string m_threadName;                        ///< 线程名称
  uint32_t m_fiberId = 0;                          ///< 协程id
  LogLevel::Level m_level = LogLevel::Level::INFO; ///< 日志等级
  const char* m_file = nullptr;                    ///< 文件名
  int32_t m_line = 0;                              ///< 行号
  std::stringstream m_ss;                          ///< 消息
  std::shared_ptr<Log> m_log;                      ///< 日志器
};

class LogEventWrap {
public:
  typedef std::shared_ptr<LogEventWrap> ptr;

  LogEventWrap(LogEvent::ptr event);
  ~LogEventWrap();

  LogEvent::ptr getEvent() const { return m_event; }

  const std::stringstream& getSS() { return m_event->getSS(); }

private:
  LogEvent::ptr m_event;
};

/**
 * @brief 日志格式器
 */
class LogFormat {
public:
  typedef std::shared_ptr<LogFormat> ptr;

  LogFormat(const std::string& pattern);

  const std::string& getPattern() const { return m_pattern; }

public:
  class FormatItem {
  public:
    typedef std::shared_ptr<FormatItem> ptr;

    virtual ~FormatItem() {}
    virtual void format(std::ostream& os, LogEvent::ptr event) = 0;
  };

public:
  void init();
  bool isError() const { return m_error; }

  virtual std::ostream& format(std::ostream& os, LogEvent::ptr event);
  virtual std::string format(LogEvent::ptr event);
private:
  std::string m_pattern;
  std::vector<FormatItem::ptr> m_items;
  bool m_error = false;
};

/**
 * @brief 日志输出地
 */
class LogAppender {
public:
  typedef std::shared_ptr<LogAppender> ptr;
  typedef int Lock;

  virtual ~LogAppender() {}
  virtual void log(
    std::shared_ptr<Log> log,
    LogLevel::Level level,
    LogEvent::ptr event) = 0;

  LogLevel::Level getLevel() const { return m_level; }
  void setLevel(LogLevel::Level val) { m_level = val; }

  LogFormat::ptr getFormatter();
  void setFormatter(LogFormat::ptr val);

protected:
  LogLevel::Level m_level = LogLevel::DEBUG;
  bool m_hasFormatter = false;
  LogFormat::ptr m_formatter;
  Lock m_lock;
};

class Log : public std::enable_shared_from_this<Log> {
public:
  typedef std::shared_ptr<Log> ptr;
  typedef int Lock;

  Log(const std::string& name);

  LogLevel::Level getLevel() const { return m_level; };
  const std::string& getName() const { return m_name; }
  void addAppender(LogAppender::ptr val);
  void delAppender(LogAppender::ptr val);
  void clearAppender();

  LogFormat::ptr getFormat();
  void setFormat(LogFormat::ptr format);

  void log(LogLevel::Level level, LogEvent::ptr e);

  void trace(LogEvent::ptr event);
  void debug(LogEvent::ptr event);
  void info(LogEvent::ptr event);
  void warn(LogEvent::ptr event);
  void error(LogEvent::ptr event);
  void fatal(LogEvent::ptr event);

private:
  std::string m_name;
  std::list<LogAppender::ptr> m_appenders;
  LogFormat::ptr m_formatter;
  LogLevel::Level m_level = LogLevel::INFO;
  Log::ptr m_root;
  Lock m_lock;
};

class LogManager {
public:
  LogManager();

  Log::ptr getRoot();
  Log::ptr getLog(const std::string& name);



private:
  Log::ptr m_root;
  std::unordered_map<std::string, Log::ptr> m_logs;
};

class StdoutAppender : public LogAppender {
public:
  typedef std::shared_ptr<StdoutAppender> ptr;

  void log(
    std::shared_ptr<Log> log,
    LogLevel::Level level,
    LogEvent::ptr event)  override;
};

class FileAppender : public LogAppender {
public:
  typedef std::shared_ptr<FileAppender> ptr;

  FileAppender(const std::string& name);

  void log(
  std::shared_ptr<Log> log,
  LogLevel::Level level,
  LogEvent::ptr event)  override;

  bool reopen();

private:
  std::string m_filename;
  std::ofstream m_filestream;
  uint64_t m_lastTime = 0;
  int m_size = 0;
};

// 网络的Socket待实现
class NetAppender : public LogAppender {

private:
};


typedef Singleton<LogManager> LogMgr;


#define LOG_ROOT LogMgr::GetInstance()->getRoot()
#define LOG_NAME(name) LogMgr::GetInstance()->getLog(name)

#define LOG_LEVEL_STREAM(log, level) \
  if (level>=log->getLevel()) \
    LogEventWrap(LogEvent::ptr(new LogEvent( \
      log, \
      level, \
      __FILE__, \
      __LINE__, \
      0, \
      0, \
      0, \
      "thread_name" \
    )))->getSS()

#define LOG_LEVEL_FMT(log, level, fmt, ...) \
  if (level>=log->getLevel()) \
    LogEventWrap(LogEvent::ptr(new LogEvent( \
      log, \
      level, \
      __FILE__, \
      __LINE__, \
      0, \
      0, \
      0, \
      "thread_name" \
    ))).getEvent()->format(fmt, __VA_ARGS__)


#define LOG_TRACE(fmt, ...) LOG_LEVEL_FMT(LOG_ROOT, LogLevel::TRACE, fmt, __VA_ARGS__)
#define LOG_DEBUG(fmt, ...) LOG_LEVEL_FMT(LOG_ROOT, LogLevel::DEBUG, fmt, __VA_ARGS__)
#define LOG_INFO(fmt, ...)  LOG_LEVEL_FMT(LOG_ROOT, LogLevel::INFO, fmt, __VA_ARGS__)
#define LOG_WARN(fmt, ...)  LOG_LEVEL_FMT(LOG_ROOT, LogLevel::WARN, fmt, __VA_ARGS__)
#define LOG_ERROR(fmt, ...) LOG_LEVEL_FMT(LOG_ROOT, LogLevel::ERROR, fmt, __VA_ARGS__)
#define LOG_FATAL(fmt, ...) LOG_LEVEL_FMT(LOG_ROOT, LogLevel::FATAL, fmt, __VA_ARGS__)


#define LOG_TRACE_STREAM LOG_LEVEL(LOG_ROOT, LogLevel::TRACE)
#define LOG_DEBUG_STREAM LOG_LEVEL(LOG_ROOT, LogLevel::DEBUG)
#define LOG_INFO_STREAM  LOG_LEVEL(LOG_ROOT, LogLevel::INFO)
#define LOG_WARN_STREAM  LOG_LEVEL(LOG_ROOT, LogLevel::WARN)
#define LOG_ERROR_STREAM LOG_LEVEL(LOG_ROOT, LogLevel::ERROR)
#define LOG_FATAL_STREAM LOG_LEVEL(LOG_ROOT, LogLevel::FATAL)