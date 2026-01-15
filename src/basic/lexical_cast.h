#pragma once

#include <yaml-cpp/yaml.h>

#include <cctype>
#include <charconv>
#include <list>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "basic/log.h"
#include "basic/utils.h"

namespace Basic {

// ============================================================================
// 基础类型转换器
// ============================================================================

template <class S, class T>
class LexicalCast {
public:
  T operator()(const S& v) {
    if constexpr (std::is_same_v<S, T>) return v;
    if constexpr (std::is_same_v<S, std::string>) {
      return from_string(v);
    } else if constexpr (std::is_same_v<T, std::string>) {
      return to_string(v);
    } else if constexpr (std::is_arithmetic_v<S> && std::is_arithmetic_v<T>) {
      if constexpr (std::is_same_v<T, bool>)
        return v != S(0);
      else if constexpr (std::is_same_v<S, bool>)
        return v ? T(1) : T(0);
      else
        return static_cast<T>(v);
    }
    LOG_ERROR("不支持类型转换 S: %s, T: %s", type_name<S>().c_str(), type_name<T>().c_str());
    return T{};
  }

private:
  T from_string(const std::string& s) {
    T val{};
    if constexpr (std::is_same_v<T, bool>) {
      if (s == "true" || s == "1" || s == "TRUE" || s == "True") return true;
      if (s == "false" || s == "0" || s == "FALSE" || s == "False") return false;
      LOG_ERROR("无效Bool类型:%s", s.c_str());
    } else if constexpr (std::is_arithmetic_v<T>) {
      auto [p, ec] = std::from_chars(s.data(), s.data() + s.size(), val);
      if (ec != std::errc{}) { LOG_ERROR("字符串类型转换%s失败", type_name<T>().c_str()); }
      return val;
    } else {
      LOG_ERROR("不支持从字符串转换到%s", type_name<T>().c_str());
    }
    return val;
  }

  std::string to_string(const S& v) {
    if constexpr (std::is_same_v<S, bool>) {
      return v ? "true" : "false";
    } else if constexpr (std::is_arithmetic_v<S>) {
      char buf[64];
      auto [p, ec] = std::to_chars(buf, buf + sizeof(buf), v);
      if (ec == std::errc{}) { return std::string(buf, p); }
      return std::to_string(v);
    } else {
      LOG_ERROR("不支持%s转换到字符串", type_name<S>().c_str());
    }
    return "";
  }
};

// ============================================================================
// 便捷函数
// ============================================================================

template <typename S, typename T>
T lexical_cast(const S& v) {
  return LexicalCast<S, T>{}(v);
}

template <typename T, typename S>
T lexical_cast2(const S& v) {
  return LexicalCast<S, T>{}(v);
}

// YAML string
// ============================================================================
// STL 容器转换器 - std::vector
// ============================================================================

template <class T>
class LexicalCast<std::string, std::vector<T>> {
public:
  std::vector<T> operator()(const std::string& v) {
    YAML::Node              node = YAML::Load(v);
    typename std::vector<T> vec;
    std::stringstream       ss;
    for (size_t i = 0; i < node.size(); ++i) {
      ss.str("");
      ss << node[i];
      vec.push_back(LexicalCast<std::string, T>()(ss.str()));
    }
    return vec;
  }
};

template <class S>
class LexicalCast<std::vector<S>, std::string> {
public:
  std::string operator()(const std::vector<S>& v) {
    YAML::Node node(YAML::NodeType::Sequence);
    for (auto& i : v) {
      node.push_back(YAML::Load(LexicalCast<S, std::string>()(i)));
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
  }
};

// ============================================================================
// STL 容器转换器 - std::list
// ============================================================================

template <class T>
class LexicalCast<std::string, std::list<T>> {
public:
  std::list<T> operator()(const std::string& v) {
    YAML::Node            node = YAML::Load(v);
    typename std::list<T> vec;
    std::stringstream     ss;
    for (size_t i = 0; i < node.size(); ++i) {
      ss.str("");
      ss << node[i];
      vec.push_back(LexicalCast<std::string, T>()(ss.str()));
    }
    return vec;  // 注意：原代码缺少 return 语句
  }
};

template <class S>
class LexicalCast<std::list<S>, std::string> {
public:
  std::string operator()(const std::list<S>& v) {
    YAML::Node node(YAML::NodeType::Sequence);
    for (auto& i : v) {
      node.push_back(YAML::Load(LexicalCast<S, std::string>()(i)));
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
  }
};

// ============================================================================
// STL 容器转换器 - std::set
// ============================================================================

template <class T>
class LexicalCast<std::string, std::set<T>> {
public:
  std::set<T> operator()(const std::string& v) {
    YAML::Node           node = YAML::Load(v);
    typename std::set<T> vec;
    std::stringstream    ss;
    for (size_t i = 0; i < node.size(); ++i) {
      ss.str("");
      ss << node[i];
      vec.insert(LexicalCast<std::string, T>()(ss.str()));
    }
    return vec;
  }
};

template <class S>
class LexicalCast<std::set<S>, std::string> {
public:
  std::string operator()(const std::set<S>& v) {
    YAML::Node node(YAML::NodeType::Sequence);
    for (auto& i : v) {
      node.push_back(YAML::Load(LexicalCast<S, std::string>()(i)));
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
  }
};

// ============================================================================
// STL 容器转换器 - std::unordered_set
// ============================================================================

template <class T>
class LexicalCast<std::string, std::unordered_set<T>> {
public:
  std::unordered_set<T> operator()(const std::string& v) {
    YAML::Node                     node = YAML::Load(v);
    typename std::unordered_set<T> vec;
    std::stringstream              ss;
    for (size_t i = 0; i < node.size(); ++i) {
      ss.str("");
      ss << node[i];
      vec.insert(LexicalCast<std::string, T>()(ss.str()));
    }
    return vec;
  }
};

template <class S>
class LexicalCast<std::unordered_set<S>, std::string> {
public:
  std::string operator()(const std::unordered_set<S>& v) {
    YAML::Node node(YAML::NodeType::Sequence);
    for (auto& i : v) {
      node.push_back(YAML::Load(LexicalCast<S, std::string>()(i)));
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
  }
};

// ============================================================================
// STL 容器转换器 - std::map
// ============================================================================

template <class T>
class LexicalCast<std::string, std::map<std::string, T>> {
public:
  std::map<std::string, T> operator()(const std::string& v) {
    YAML::Node                        node = YAML::Load(v);
    typename std::map<std::string, T> vec;
    std::stringstream                 ss;
    for (auto it = node.begin(); it != node.end(); ++it) {
      ss.str("");
      ss << it->second;
      vec.insert(std::make_pair(it->first.Scalar(), lexical_cast<std::string, T>(ss.str())));
    }
    return vec;
  }
};

template <class S>
class LexicalCast<std::map<std::string, S>, std::string> {
public:
  std::string operator()(const std::map<std::string, S>& v) {
    YAML::Node node(YAML::NodeType::Map);
    for (auto& i : v) {
      node[i.first] = YAML::Load(lexical_cast<S, std::string>(i.second));
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
  }
};

// ============================================================================
// STL 容器转换器 - std::unordered_map
// ============================================================================

template <class T>
class LexicalCast<std::string, std::unordered_map<std::string, T>> {
public:
  std::unordered_map<std::string, T> operator()(const std::string& v) {
    YAML::Node                                  node = YAML::Load(v);
    typename std::unordered_map<std::string, T> vec;
    std::stringstream                           ss;
    for (auto it = node.begin(); it != node.end(); ++it) {
      ss.str("");
      ss << it->second;
      vec.insert(std::make_pair(it->first.Scalar(), lexical_cast<std::string, T>(ss.str())));
    }
    return vec;  // 注意：原代码缺少 return 语句
  }
};

template <class S>
class LexicalCast<std::unordered_map<std::string, S>, std::string> {
public:
  std::string operator()(const std::unordered_map<std::string, S>& v) {
    YAML::Node node(YAML::NodeType::Map);
    for (auto& i : v) {
      node[i.first] = YAML::Load(lexical_cast<S, std::string>(i.second));
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
  }
};

}  // namespace Basic