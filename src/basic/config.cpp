
#include "basic/config.h"

#include <sys/stat.h>
#include <sys/types.h>

#include <string>

namespace Basic {

ConfigVarBase::ptr Config::LookupBase(const std::string& name) {
  auto it = GetDatas().find(name);
  return it == GetDatas().end() ? nullptr : it->second;
}

static void ListAllMember(const std::string& prefix, const YAML::Node& node,
                          std::list<std::pair<std::string, const YAML::Node> >& output) {
  if (prefix.find_first_not_of("abcdefghijklmnopqrstuvwxyz._01234567890") != std::string::npos) {
    LOG_ERROR("Config invalid name: [%s:%s]", prefix.c_str(), node.as<std::string>().c_str());
    return;
  }

  output.push_back(std::make_pair(prefix, node));
  if (node.IsMap()) {
    for (auto it = node.begin(); it != node.end(); ++it) {
      ListAllMember(prefix.empty() ? it->first.Scalar() : prefix + "." + it->first.Scalar(),
                    it->second, output);
    }
  }
}

void Config::LoadFromYaml(const YAML::Node& root) {
  std::list<std::pair<std::string, const YAML::Node> > all_nodes;
  ListAllMember("", root, all_nodes);

  for (auto& i : all_nodes) {
    std::string key = i.first;
    if (key.empty()) { continue; }

    std::transform(key.begin(), key.end(), key.begin(), ::tolower);
    ConfigVarBase::ptr var = LookupBase(key);

    if (var) {
      if (i.second.IsScalar()) {
        var->from_string(i.second.Scalar());
      } else {
        std::stringstream ss;
        ss << i.second;
        var->from_string(ss.str());
      }
    }
  }
}

static std::map<std::string, uint64_t> s_file2modifytime;

void Config::LoadFromDir(const std::string& path, bool force) {
  std::string              absoulte_path = "./config";
  std::vector<std::string> files;
  // FSUtil::ListAllFile(files, absoulte_path, ".yml");

  for (auto& i : files) {
    {
      struct stat st;
      lstat(i.c_str(), &st);
      if (!force && s_file2modifytime[i] == (uint64_t)st.st_mtime) { continue; }
      s_file2modifytime[i] = st.st_mtime;
    }
    try {
      YAML::Node root = YAML::LoadFile(i);
      LoadFromYaml(root);
      LOG_INFO("LoadConfFile file=%s ok", i.c_str());
    } catch (...) { LOG_ERROR("LoadConfFile file=%s failed", i.c_str()); }
  }
}

void Config::Visit(std::function<void(ConfigVarBase::ptr)> cb) {
  ConfigVarMap&         m = GetDatas();
  for (auto it = m.begin(); it != m.end(); ++it) {
    cb(it->second);
  }
}

}  // namespace Basic