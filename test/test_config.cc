#include "log.h"
#include "config.h"
#include <yaml-cpp/yaml.h>
using namespace sylar;

sylar::ConfigVar<int>::ptr g_int_value_config = sylar::Config::Lookup("system.port", (int)8080, "system port");

sylar::ConfigVar<float>::ptr g_float_value_config = sylar::Config::Lookup("system.value", (float)10.2f, "system value");

sylar::ConfigVar<std::vector<int>>::ptr g_vec_int_value_config = sylar::Config::Lookup("system.int_vec", std::vector<int>{1, 2}, "system int vec");

sylar::ConfigVar<std::list<int>>::ptr g_list_int_value_config = sylar::Config::Lookup("system.int_list", std::list<int>{1, 2}, "system int list");

sylar::ConfigVar<std::set<int>>::ptr g_set_int_value_config = sylar::Config::Lookup("system.int_set", std::set<int>{1, 2}, "system int set");

sylar::ConfigVar<std::unordered_set<int>>::ptr g_uset_int_value_config = sylar::Config::Lookup("system.int_uset", std::unordered_set<int>{1, 2}, "system int set");

sylar::ConfigVar<std::map<std::string, int>>::ptr g_map_int_value_config = sylar::Config::Lookup("system.int_map", std::map<std::string, int>{{"k", 2}}, "system int map");

sylar::ConfigVar<std::unordered_map<std::string, int>>::ptr g_umap_int_value_config = sylar::Config::Lookup("system.int_umap", std::unordered_map<std::string, int>{{"k1", 4}}, "system int umap");

void print_yaml(const YAML::Node &node, int level)
{
    if (node.IsScalar())
    {
        LOG_INFO(LOG_ROOT()) << std::string(level * 4, ' ')
                             << node.Scalar() << " - " << node.Type() << " - " << level;
    }
    else if (node.IsNull())
    {
        LOG_INFO(LOG_ROOT()) << std::string(level * 4, ' ')
                             << "NULL - " << node.Type() << " - " << level;
    }
    else if (node.IsMap())
    {
        for (auto it = node.begin();
             it != node.end(); ++it)
        {
            LOG_INFO(LOG_ROOT()) << std::string(level * 4, ' ')
                                 << it->first << " - " << it->second.Type() << " - " << level;
            print_yaml(it->second, level + 1);
        }
    }
    else if (node.IsSequence())
    {
        for (size_t i = 0; i < node.size(); ++i)
        {
            LOG_INFO(LOG_ROOT()) << std::string(level * 4, ' ')
                                 << i << " - " << node[i].Type() << " - " << level;
            print_yaml(node[i], level + 1);
        }
    }
}

void test_config()
{
    LOG_INFO(LOG_ROOT()) << "before: " << g_int_value_config->toString();
    LOG_INFO(LOG_ROOT()) << "before: " << g_float_value_config->toString();
    auto v = g_vec_int_value_config->getValue();
    for (auto &i : v)
    {
        LOG_INFO(LOG_ROOT()) << "before int_vec: " << i;
    }

    auto l = g_list_int_value_config->getValue();
    for (auto &i : l)
    {
        LOG_INFO(LOG_ROOT()) << "before int_list: " << i;
    }

    auto s = g_set_int_value_config->getValue();
    for (auto &i : s)
    {
        LOG_INFO(LOG_ROOT()) << "before int_set: " << i;
    }

    auto us = g_uset_int_value_config->getValue();
    for (auto &i : us)
    {
        LOG_INFO(LOG_ROOT()) << "before int_unordered_set: " << i;
    }

    auto map_int = g_map_int_value_config->getValue();
    for (auto &i : map_int)
    {
        LOG_INFO(LOG_ROOT()) << "before int_map: key: " << i.first << " val: " << i.second;
    }

    auto umap_int = g_umap_int_value_config->getValue();
    for (auto &i : umap_int)
    {
        LOG_INFO(LOG_ROOT()) << "before int_umap: key: " << i.first << " val: " << i.second;
    }

    YAML::Node root = YAML::LoadFile("/home/wu/Documents/sylar/bin/log.yml");
    Config::LoadFromYaml(root);

    LOG_INFO(LOG_ROOT()) << "after: " << g_int_value_config->getValue();
    LOG_INFO(LOG_ROOT()) << "after: " << g_float_value_config->toString();
    v = g_vec_int_value_config->getValue();
    for (auto &i : v)
    {
        LOG_INFO(LOG_ROOT()) << "after int_vec: " << i;
    }
    l = g_list_int_value_config->getValue();
    for (auto &i : l)
    {
        LOG_INFO(LOG_ROOT()) << "after int_list: " << i;
    }

    s = g_set_int_value_config->getValue();
    for (auto &i : s)
    {
        LOG_INFO(LOG_ROOT()) << "after int_set: " << i;
    }

    us = g_uset_int_value_config->getValue();
    for (auto &i : us)
    {
        LOG_INFO(LOG_ROOT()) << "after int_unordered_set: " << i;
    }

    map_int = g_map_int_value_config->getValue();
    for (auto &i : map_int)
    {
        LOG_INFO(LOG_ROOT()) << "after int_map: key: " << i.first << " val: " << i.second;
    }

    umap_int = g_umap_int_value_config->getValue();
    for (auto &i : umap_int)
    {
        LOG_INFO(LOG_ROOT()) << "after int_umap: key: " << i.first << " val: " << i.second;
    }
}

void test_yaml()
{
    YAML::Node root = YAML::LoadFile("/home/wu/Documents/sylar/bin/log.yml");
    // if (!root["system"])
    // {
    //     LOG_ERROR(LOG_ROOT()) << "config.yaml not found system";
    //     return;
    // }
    // if (!root["system"]["port"])
    // {
    //     LOG_ERROR(LOG_ROOT()) << "config.yaml not found system.port";
    //     return;
    // }
    // g_int_value_config->fromString(root["system"]["port"].as<std::string>());
    // LOG_INFO(LOG_ROOT()) << root;
    print_yaml(root, 0);
}

int main(int argc, char *argv[])
{

    // LOG_INFO(LOG_ROOT()) << g_int_value_config->getValue();
    // LOG_INFO(LOG_ROOT()) << g_int_value_config->toString();

    // test_yaml();
    test_config();
}