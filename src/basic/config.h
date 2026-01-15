#pragma once

#include <yaml-cpp/yaml.h>

#include <algorithm>
#include <cctype>
#include <exception>
#include <functional>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

#include "basic/lexical_cast.h"
#include "basic/log.h"
#include "basic/mutex.h"
#include "basic/utils.h"

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

template <class T, class FromStr = LexicalCast<std::string, T>,
          class ToStr = LexicalCast<T, std::string>>
class ConfigVar : public ConfigVarBase {
public:
  typedef std::shared_ptr<ConfigVar>                                  ptr;
  typedef std::function<void(const T& old_value, const T& new_value)> on_change_cb;
  typedef RWMutex                                                     LockType;

  ConfigVar(const std::string& name, const T& default_value, const std::string& desc = "")
      : ConfigVarBase(name, desc), m_val(default_value) {}

  std::string to_string() override {
    try {
      LockType::ReadLock lock(m_lock);
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
      LockType::ReadLock lock(m_lock);
      if (v == m_val) return;
      for (auto& i : m_cbs) {
        i.second(m_val, v);
      }
    }
    LockType::WriteLock lock(m_lock);
    m_val = v;
  }

  const T getValue() {
    LockType::ReadLock lock(m_lock);
    return m_val;
  }

  std::string getTypeName() const override { return type_name<T>(); }

  uint64_t addListener(on_change_cb cb) {
    static uint64_t     s_fun_id = 0;
    LockType::WriteLock lock(m_lock);
    ++s_fun_id;
    m_cbs[s_fun_id] = cb;
    return s_fun_id;
  }

  void delListener(uint64_t key) {
    LockType::WriteLock lock(m_lock);
    m_cbs.erase(key);
  }

  on_change_cb getListener(uint64_t key) {
    LockType::ReadLock lock(m_lock);
    auto               it = m_cbs.find(key);
    return it == m_cbs.end() ? nullptr : it->second;
  }

  void clearListener() {
    LockType::WriteLock lock(m_lock);
    m_cbs.clear();
  }

private:
  LockType                         m_lock;
  T                                m_val;
  std::map<uint64_t, on_change_cb> m_cbs;
};

class Config {
public:
  typedef std::unordered_map<std::string, ConfigVarBase::ptr> ConfigVarMap;
  typedef RWMutex                                             LockType;

  template <class T>
  static typename ConfigVar<T>::ptr Lookup(const std::string& name, const T& default_value,
                                           const std::string& desc = "") {
    LockType::WriteLock lock(GetLock());
    auto                it = GetDatas().find(name);
    if (it != GetDatas().end()) {
      auto tmp = std::dynamic_pointer_cast<ConfigVar<T>>(it->second);
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
    return std::dynamic_pointer_cast<ConfigVar<T>>(it->second);
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

  static LockType& GetLock() {
    static LockType s_lock;
    return s_lock;
  }
};

}  // namespace Basic