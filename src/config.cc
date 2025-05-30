#include "config.h"

namespace sylar
{
    // "a.b", 10
    // a:
    //   b: 10

    static void ListAllMember(const std::string &prefix,
                              const YAML::Node &node,
                              std::list<std::pair<std::string, YAML::Node>> &out)
    {
        if (prefix.find_first_not_of("abcdefghijklmnopqrstuvwxyz._0123456789") != std::string::npos)
        {
            LOG_ERROR(LOG_ROOT()) << "config invalid name: " << prefix << " : " << node;
            return;
        }
        out.push_back(std::make_pair(prefix, node));
        if (node.IsMap())
        {
            for (auto it = node.begin(); it != node.end(); ++it)
            {
                ListAllMember(prefix.empty() ? it->first.Scalar() : prefix + "." + it->first.Scalar(), it->second, out);
            }
        }
    }

    ConfigVarBase::ptr Config::LookupBase(const std::string &name)
    {
        RWMutexType::ReadLock lock(GetMutex());
        auto it = GetDatas().find(name);
        return it == GetDatas().end() ? nullptr : it->second;
    }

    void Config::LoadFromYaml(const YAML::Node &node)
    {
        std::list<std::pair<std::string, YAML::Node>> all_nodes;
        ListAllMember("", node, all_nodes);

        for (auto &item : all_nodes)
        {
            std::string key = item.first;
            if (key.empty())
                continue;

            std::transform(key.begin(), key.end(), key.begin(), ::tolower);
            ConfigVarBase::ptr var = LookupBase(key);

            if (var)
            {
                if (item.second.IsScalar())
                {
                    var->fromString(item.second.Scalar());
                }
                else
                {
                    std::stringstream ss;
                    ss << item.second;
                    var->fromString(ss.str());
                }
            }
        }
    }

    void Config::Visit(std::function<void(ConfigVarBase::ptr)> cb)
    {
        RWMutexType::ReadLock lock(GetMutex());
        ConfigVarMap &m = GetDatas();
        for (auto it = m.begin(); it != m.end(); it++)
        {
            cb(it->second);
        }
    }
}