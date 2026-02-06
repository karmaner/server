#include "basic/env.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <iomanip>
#include <iostream>

#include "basic/log.h"

namespace Basic {

bool Env::init(int argc, char* argv[]) {
  char link[1024] = {0};
  char path[1024] = {0};

  sprintf(link, "/proc/%d/exe", getpid());
  size_t len = readlink(link, path, sizeof(path));
  if (len == -1) {
    perror("readlink failed");
    return false;
  }
  path[len] = '\0';

  m_exe     = path;

  auto pos = m_exe.find_last_of("/");
  m_cwd    = m_exe.substr(0, pos) + "/";

  m_program = argv[0];

  const char* now_key = nullptr;
  for (int i = 1; i < argc; ++i) {
    if (argv[i][0] == '-') {
      if (strlen(argv[i]) > 1) {
        if (now_key) { add(now_key, ""); }
        now_key = argv[i] + 1;
      } else {
        now_key = argv[i] + 1;
        LOG_ERROR_STREAM << "invalid arg idx=" << i << " val=" << argv[i];
        return false;
      }
    } else {
      if (now_key) {
        add(now_key, argv[i]);
        now_key = nullptr;
      } else {
        LOG_ERROR_STREAM << "invalid arg idx=" << i << " val=" << argv[i];
        return false;
      }
    }
  }
  if (now_key) { add(now_key, ""); }
  return true;
}

void Env::add(const std::string& key, const std::string& val) {
  LockType::WriteLock lock(m_lock);
  m_args[key] = val;
}

bool Env::has(const std::string& key) {
  LockType::ReadLock lock(m_lock);
  return m_args.find(key) != m_args.end();
}

void Env::del(const std::string& key) {
  LockType::WriteLock lock(m_lock);
  m_args.erase(key);
}

std::string Env::get(const std::string& key, const std::string& default_value) {
  LockType::ReadLock lock(m_lock);
  return m_args.find(key) != m_args.end() ? m_args[key] : default_value;
}

void Env::addHelp(const std::string& key, const std::string& desc) {
  removeHelp(key);
  LockType::WriteLock lock(m_lock);
  m_helps.push_back(std::make_pair(key, desc));
}

void Env::removeHelp(const std::string& key) {
  LockType::WriteLock lock(m_lock);
  for (auto it = m_helps.begin(); it != m_helps.end();) {
    if (it->first == key) {
      it = m_helps.erase(it);
    } else {
      ++it;
    }
  }
}

void Env::printHelp() {
  LockType::ReadLock lock(m_lock);
  std::cout << "Usage: " << m_program << " [options]" << std::endl;
  for (auto& i : m_helps) {
    std::cout << std::setw(5) << "-" << i.first << " : " << i.second << std::endl;
  }
}

bool Env::setEnv(const std::string& key, const std::string& val) {
  return !setenv(key.c_str(), val.c_str(), 1);
}

std::string Env::getEnv(const std::string& key, const std::string& default_value) {
  const char* v = getenv(key.c_str());
  if (v == nullptr) { return default_value; }
  return v;
}

std::string Env::getAbsolutePath(const std::string& path) const {
  if (path.empty()) { return "/"; }
  if (path[0] == '/') { return path; }
  return m_cwd + path;
}

std::string Env::getConfigPath() {
  return getAbsolutePath(get("c", "conf"));
}

}  // namespace Basic