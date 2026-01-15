#include <chrono>
#include <iostream>
#include <map>

#include "server.h"

using namespace Basic;

void test_cast() {
  class A {};
  LOG_INFO("A type is: %s", type_name<A>().c_str());

  // 基础类型转换测试
  LOG_INFO("=== 基础类型转换测试 ===");

  // 字符串 -> 数值类型
  int a = lexical_cast2<int>("123");
  LOG_INFO("string->int: '123' -> %d", a);

  double b = lexical_cast2<double>("3.14");
  LOG_INFO("string->double: '3.14' -> %.2f", b);

  bool c = lexical_cast<std::string, bool>("1");
  LOG_INFO("string->bool: '1' -> %s", c ? "true" : "false");

  bool d = lexical_cast<std::string, bool>("0");
  LOG_INFO("string->bool: '0' -> %s", d ? "true" : "false");

  // 数值类型 -> 字符串
  std::string e = lexical_cast<int, std::string>(456);
  LOG_INFO("int->string: 456 -> '%s'", e.c_str());

  std::string f = lexical_cast<double, std::string>(2.718);
  LOG_INFO("double->string: 2.718 -> '%s'", f.c_str());

  std::string g = lexical_cast<bool, std::string>(true);
  LOG_INFO("bool->string: true -> '%s'", g.c_str());

  // 数值类型之间的转换
  LOG_INFO("=== 数值类型之间转换测试 ===");

  double h = lexical_cast<int, double>(100);
  LOG_INFO("int->double: 100 -> %.1f", h);

  int i = lexical_cast<double, int>(99.9);
  LOG_INFO("double->int: 99.9 -> %d", i);

  bool j = lexical_cast<int, bool>(42);
  LOG_INFO("int->bool: 42 -> %s", j ? "true" : "false");

  int k = lexical_cast<bool, int>(false);
  LOG_INFO("bool->int: false -> %d", k);

  // 相同类型转换（应该直接返回）
  LOG_INFO("=== 相同类型转换测试 ===");

  std::string same_str = lexical_cast<std::string, std::string>("hello");
  LOG_INFO("string->string: 'hello' -> '%s'", same_str.c_str());

  int same_int = lexical_cast<int, int>(999);
  LOG_INFO("int->int: 999 -> %d", same_int);

  // 边界值和特殊值测试
  LOG_INFO("=== 边界值和特殊值测试 ===");

  try {
    int zero = lexical_cast<std::string, int>("0");
    LOG_INFO("string->int: '0' -> %d", zero);

    int negative = lexical_cast<std::string, int>("-42");
    LOG_INFO("string->int: '-42' -> %d", negative);

    bool false_bool = lexical_cast<std::string, bool>("false");
    LOG_INFO("string->bool: 'false' -> %s", false_bool ? "true" : "false");

    bool False_bool = lexical_cast<std::string, bool>("False");
    LOG_INFO("string->bool: 'False' -> %s", False_bool ? "true" : "false");

    bool FALSE_bool = lexical_cast<std::string, bool>("FALSE");
    LOG_INFO("string->bool: 'FALSE' -> %s", FALSE_bool ? "true" : "false");

    bool true_bool = lexical_cast<std::string, bool>("true");
    LOG_INFO("string->bool: 'true' -> %s", true_bool ? "true" : "false");

  } catch (const std::exception& e) { LOG_ERROR("转换异常: %s", e.what()); }

  // 错误处理测试
  LOG_INFO("=== 错误处理测试 ===");

  try {
    int invalid = lexical_cast<std::string, int>("abc");
    LOG_INFO("无效字符串转换: 'abc' -> %d", invalid);
  } catch (const std::exception& e) { LOG_ERROR("预期中的错误: %s", e.what()); }

  try {
    // 测试不支持的类型转换
    std::vector<int> vec;
    auto             result = lexical_cast<std::vector<int>, std::string>(vec);
  } catch (const std::exception& e) { LOG_ERROR("预期中的错误: %s", e.what()); }

  // 性能测试样例
  LOG_INFO("=== 性能测试样例 ===");

  const int TEST_COUNT = 1000;
  auto      start      = std::chrono::high_resolution_clock::now();

  for (int i = 0; i < TEST_COUNT; ++i) {
    auto result = lexical_cast<int, std::string>(i);
  }

  auto end      = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
  LOG_INFO("%d 次 int->string 转换耗时: %lld 微秒", TEST_COUNT, duration.count());

  LOG_INFO("=== 所有测试完成 ===");
}

void test_yaml() {
  // 1. 基础数据类型
  auto int_var = Config::Lookup<int>("system.port", 8080, "系统端口");

  auto bool_var   = Config::Lookup<bool>("system.debug", false, "是否开启调试模式");
  auto float_var  = Config::Lookup<float>("system.timeout", 5.5f, "超时时间");
  auto string_var = Config::Lookup<std::string>("system.name", "default_server", "系统名称");

  // 2. 创建容器类型的配置变量
  std::vector<int> default_ports = {80, 443, 8080};
  auto vec_var = Config::Lookup<std::vector<int>>("server.ports", default_ports, "服务端口列表");

  std::map<std::string, int> default_weights = {{"master", 10}, {"slave", 5}};
  auto                       map_var =
      Config::Lookup<std::map<std::string, int>>("server.weights", default_weights, "服务器权重");

  // 3. 测试序列化为字符串
  std::cout << "Initial values:" << std::endl;
  std::cout << "system.port = " << int_var->getValue() << std::endl;
  std::cout << "system.debug = " << bool_var->getValue() << std::endl;
  std::cout << "server.ports = " << vec_var->to_string() << std::endl;
  std::cout << "server.weights = " << map_var->to_string() << std::endl;

  // 4. 创建YAML节点并设置值
  YAML::Node root;
  root["system"]["port"]    = 9090;
  root["system"]["debug"]   = true;
  root["system"]["timeout"] = 10.5f;
  root["system"]["name"]    = "test_server";

  std::vector<int> ports_node;
  ports_node.push_back(8000);
  ports_node.push_back(8080);
  ports_node.push_back(8888);
  root["server"]["ports"] = ports_node;

  std::map<std::string, int> weights_node;
  weights_node["master"]    = 20;
  weights_node["backup"]    = 15;
  root["server"]["weights"] = weights_node;

  // 5. 从YAML加载配置
  std::cout << "\nLoading config from YAML..." << std::endl;
  Basic::Config::LoadFromYaml(root);

  // 6. 验证加载后的值
  std::cout << "\nValues after loading from YAML:" << std::endl;
  std::cout << "system.port = " << int_var->to_string() << " (expected: 9090)" << std::endl;
  std::cout << "system.debug = " << bool_var->to_string() << " (expected: true)" << std::endl;
  std::cout << "system.timeout = " << float_var->to_string() << " (expected: 10.5)" << std::endl;
  std::cout << "system.name = " << string_var->to_string() << " (expected: test_server)"
            << std::endl;
  std::cout << "server.ports = " << vec_var->to_string() << " (expected: [8000, 8080, 8888])"
            << std::endl;
  std::cout << "server.weights = " << map_var->to_string()
            << " (expected: {backup: 15, master: 20})" << std::endl;

  // 7. 回调函数测试
  std::cout << "\nTesting callbacks..." << std::endl;
  bool callback_triggered = false;
  int_var->addListener([&callback_triggered](const int& old_value, const int& new_value) {
    std::cout << "Port changed from " << old_value << " to " << new_value << std::endl;
    callback_triggered = true;
  });

  // 修改值，触发回调
  int_var->setValue(9999);
  std::cout << "system.port after change = " << int_var->to_string() << " (expected: 9999)"
            << std::endl;
  std::cout << "Callback triggered: " << (callback_triggered ? "yes" : "no") << std::endl;

  // 8. 将配置转为完整的YAML
  std::cout << "\nComplete config as YAML:" << std::endl;
  YAML::Node full_config;

  // 收集所有配置项
  Basic::Config::Visit([&full_config](Basic::ConfigVarBase::ptr var) {
    std::string name    = var->getName();
    size_t      pos     = name.find('.');
    YAML::Node  current = full_config;

    // 处理多级配置项，如 system.port
    if (pos != std::string::npos) {
      std::string first_level  = name.substr(0, pos);
      std::string second_level = name.substr(pos + 1);

      if (!full_config[first_level]) { full_config[first_level] = YAML::Node(YAML::NodeType::Map); }
      full_config[first_level][second_level] = YAML::Load(var->to_string());
    } else {
      full_config[name] = YAML::Load(var->to_string());
    }
  });

  std::cout << full_config << std::endl;
  std::cout << "文件输出完毕" << std::endl;

  // 9. 保存到文件
  std::ofstream fout("./test_config.yml");
  if (fout.is_open()) {
    fout << full_config;
    fout.close();
    std::cout << "\nConfiguration saved to test_config.yml" << std::endl;

    // 10. 从文件重新加载验证
    std::cout << "Reloading from file to verify..." << std::endl;
    YAML::Node file_root = YAML::LoadFile("test_config.yml");
    Basic::Config::LoadFromYaml(file_root);

    std::cout << "system.port after reload = " << int_var->to_string() << std::endl;
    std::cout << "server.weights after reload = " << map_var->to_string() << std::endl;
  } else {
    std::cerr << "Failed to open file for writing" << std::endl;
  }

  // 11. 测试不存在的配置项
  std::cout << "\nTesting non-existent config item..." << std::endl;
  auto not_exists = Basic::Config::Lookup<int>("non.existent.item");
  if (!not_exists) {
    std::cout << "Correctly handled non-existent configuration item" << std::endl;
  } else {
    std::cout << "ERROR: Should not find non-existent configuration item" << std::endl;
  }

  std::cout << "\nYAML config test completed!" << std::endl;
}

void test_log_file_config() {
  // 测试加载文件
  std::cout << "=== Testing log config file loading ===" << std::endl;

  // 假设有一个配置文件 log.yaml
  Config::LoadFromDir("", true);

  // 获取当前的日志配置
  ConfigVar<std::set<LogDefine>>::ptr logs        = Config::Lookup<std::set<LogDefine>>("logs");
  auto                                log_defines = logs->getValue();
  std::cout << "Loaded " << log_defines.size() << " log from config file" << std::endl;

  for (const auto& log_define : log_defines) {
    std::cout << "Logger: " << log_define.name
              << ", Level: " << LogLevel::to_string(log_define.level)
              << ", Formatters: " << log_define.formatter << std::endl;

    for (const auto& appender : log_define.appenders) {
      std::string appender_type = (appender.type == 1)   ? "StdoutAppender"
                                  : (appender.type == 2) ? "FileAppender"
                                                         : "Unknown";
      std::cout << "  - Appender: " << appender_type
                << ", Level: " << LogLevel::to_string(appender.level)
                << ", Formatter: " << appender.formatter;
      if (appender.type == 2) { std::cout << ", File: " << appender.file; }
      std::cout << std::endl;
    }
  }

  // 导出配置到文件
  std::ofstream ofs("./exported_log.yaml");
  if (ofs.is_open()) {
    std::string yaml_str = LogMgr::GetInstance()->toYamlString();
    std::cout << yaml_str << std::endl;
    ofs << yaml_str;

    std::string yaml_str2 = logs->to_string();
    std::cout << yaml_str2;
    ofs.close();
    std::cout << "Successfully exported log config to ./exported_log.yaml" << std::endl;
    std::cout << "Exported content:\n" << yaml_str << std::endl;
  } else {
    std::cerr << "Failed to open file for export" << std::endl;
  }
}

int main() {
  test_cast();

  test_yaml();

  test_log_file_config();

  return 0;
}