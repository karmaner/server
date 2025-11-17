#include <yaml-cpp/yaml.h>

#include <algorithm>
#include <cctype>
#include <charconv>
#include <exception>
#include <functional>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "basic/log.h"
#include "utils/type_util.h"
#include "yaml-cpp/node/parse.h"
#include "yaml-cpp/node/type.h"

namespace Basic {

class ConfigVarBase {
public:
  typedef std::shared_ptr<ConfigVarBase> ptr;

  ConfigVarBase(const std::string& name, const std::string& desc = "")
      : m_name(name), m_desc(desc) {
    std::transform(m_name.begin(), m_name.end(), m_name.begin(), ::tolower);
  }

  virtual ~ConfigVarBase() {}

  const std::string& getName() const { return m_name; }
  const std::string& getDesc() const { return m_desc; }

  virtual std::string to_string()                         = 0;
  virtual bool        from_string(const std::string& val) = 0;

  virtual std::string getTypeName() const = 0;

protected:
  std::string m_desc;  // 描述
  std::string m_name;  // 配置名称
};

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
      if (ec != std::errc{}) {
        LOG_ERROR("字符串类型转换%s失败", type_name<T>().c_str(), type_name(s).c_str());
      }
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

template <typename S, typename T>
T lexical_cast(const S& v) {
  return LexicalCast<S, T>{}(v);
}

template <class T>
class LexicalCast<std::string, std::vector<T> > {
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

template <class T>
class LexicalCast<std::string, std::list<T> > {
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

template <class T>
class LexicalCast<std::string, std::set<T> > {
public:
  std::string operator()(const std::set<T>& v) {
    YAML::Node node(YAML::NodeType::Sequence);
    for (auto& i : v) {
      node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
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

template <class T>
class LexicalCast<std::string, std::unordered_set<T> > {
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

template <class T>
class LexicalCast<std::string, std::map<std::string, T> > {
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

template <class T>
class LexicalCast<std::string, std::unordered_map<std::string, T> > {
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

template <class T, class FromStr = LexicalCast<std::string, T>,
          class ToStr = LexicalCast<T, std::string> >
class ConfigVar : public ConfigVarBase {
public:
  typedef std::shared_ptr<ConfigVar>                                  ptr;
  typedef std::function<void(const T& old_value, const T& new_value)> on_change_cb;
  typedef int                                                         Lock;

  ConfigVar(const std::string& name, const T& default_value, const std::string& desc = "")
      : ConfigVarBase(name, desc), m_val(default_value) {}

  std::string to_string() override {
    try {
      return ToStr()(m_val);
    } catch (std::exception& e) {
      LOG_ERROR("ConfigVar::toString exception:%s, convert:%s, name=%s", e.what(),
                type_name(m_val).c_str(), m_name.c_str());
      return "";
    }
  }

  bool from_string(const std::string& val) override {
    try {
      setValue(FromStr()(val));
      return true;
    } catch (std::exception& e) {
      LOG_ERROR("ConfigVar::fromString exception: %s, convert: string to %s, name=%s, val=%s",
                e.what(), type_name<T>().c_str(), m_name.c_str(), val.c_str());
    }
    return false;
  }

  void setValue(const T& v) {
    {
      Lock lock;
      if (v == m_val) return;
      for (auto& i : m_cbs) {
        i.second(m_val, v);
      }
    }
    m_val = v;
  }

  const T getValue() { return m_val; }

  std::string getTypeName() const override { return type_name<T>(); }

  uint64_t addListener(on_change_cb cb) {
    static uint64_t s_fun_id = 0;
    Lock            lock;  // (m_mutex);
    ++s_fun_id;
    m_cbs[s_fun_id] = cb;
    return s_fun_id;
  }

  void delListener(uint64_t key) {
    Lock lock;
    m_cbs.erase(key);
  }

  on_change_cb getListener(uint64_t key) {
    auto it = m_cbs.find(key);
    return it == m_cbs.end() ? nullptr : it->second;
  }

  void clearListener() { m_cbs.clear(); }

private:
  Lock                             m_mutex;
  T                                m_val;
  std::map<uint64_t, on_change_cb> m_cbs;
};

class Config {
public:
  typedef std::unordered_map<std::string, ConfigVarBase::ptr> ConfigVarMap;
  typedef int                                                 RWLock;

  template <class T>
  static typename ConfigVar<T>::ptr Lookup(const std::string& name, const T& default_value,
                                           const std::string& desc = "") {
    auto it = GetDatas().find(name);
    if (it != GetDatas().end()) {
      auto tmp = std::dynamic_pointer_cast<ConfigVar<T> >(it->second);
      if (tmp) {
        LOG_INFO("Lookup name=%s exists;", name.c_str());
        return tmp;
      } else {
        LOG_ERROR("Lookup name=%s exists but type not. type=%s, real_type=%s", name.c_str(),
                  type_name<T>().c_str(), it->second->getTypeName().c_str());
        return nullptr;
      }
    }

    if (name.find_first_not_of("abcdefghijklmnopqrstuvwxyz._01234567890") != std::string::npos) {
      LOG_ERROR("Lookup name invalid name=", name.c_str());
      throw std::invalid_argument(name);
    }

    typename ConfigVar<T>::ptr v(new ConfigVar<T>(name, default_value, desc));
    GetDatas()[name] = v;
    return v;
  }

  template <class T>
  static typename ConfigVar<T>::ptr Lookup(const std::string& name) {
    auto it = GetDatas().find(name);
    if (it == GetDatas().end()) { return nullptr; }
    return std::dynamic_pointer_cast<ConfigVar<T> >(it->second);
  }

  static ConfigVarBase::ptr LookupBase(const std::string& name);
  static void               Visit(std::function<void(ConfigVarBase::ptr)> cb);

  static void LoadFromYaml(const YAML::Node& root);
  static void LoadFromDir(const std::string& path, bool force = false);

private:
  static ConfigVarMap& GetDatas() {
    static ConfigVarMap s_datas;
    return s_datas;
  }
};

}  // namespace Basic