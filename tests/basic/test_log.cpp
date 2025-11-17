#include <string>

#include "basic/log.h"

using namespace Basic;

void test_log() {
  LOG_INFO("基础功能");
  LOG_TRACE("详细日志不输入，因为Log默认时DEBUG");
  LOG_INFO("Hello, %s", "World!");
  LOG_DEBUG("Is All Good %d\n, Maybe It can do that shit %s", 50, "Cool");
  LOG_ERROR("错误日志输出");
  LOG_INFO_STREAM << "All good ?" << std::endl;
}

void test_appender() {
  LOG_INFO("开始测试");
  LogAppender::ptr appender1(new StdoutAppender);

  std::string      file_path = "./test_file";
  LogAppender::ptr appender2(new FileAppender(file_path));

  Log::ptr test_log = LogMgr::GetInstance()->getLog("test");
  LOG_INFO("添加appender");
  test_log->addAppender(appender1);
  test_log->addAppender(appender2);

  LOG_LEVEL_FMT(test_log, LogLevel::DEBUG, "test info %s", "ok?");
}

void test_format() {
  LOG_INFO("开始测试");

  std::string      file_path = "./test_file";
  LogAppender::ptr appender(new FileAppender(file_path));

  LogFormat::ptr format(new LogFormat("%d, %m%n"));

  appender->setFormatter(format);

  LOG_ROOT->addAppender(appender);

  LOG_INFO("After Modify");
}

int main() {
  test_log();

  test_appender();

  test_format();
  return 0;
}