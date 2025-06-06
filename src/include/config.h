#pragma once
#include <memory>
#include <sstream>
#include <string>
#include <vector>
#include <set>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <functional>
#include <boost/lexical_cast.hpp>
#include <yaml-cpp/yaml.h>
#include "log.h"
#include "mutex.h"

namespace sylar
{

    class ConfigVarBase
    {
    public:
        typedef std::shared_ptr<ConfigVarBase> ptr;
        ConfigVarBase(const std::string &name, const std::string &description = "")
            : m_name(name), m_description(description)
        {
            std::transform(m_name.begin(), m_name.end(), m_name.begin(), ::tolower);
        }

        ~ConfigVarBase() {}

        const std::string &getName() const { return m_name; }

        const std::string &getDescription() const { return m_description; }

        virtual std::string toString() = 0;

        virtual bool fromString(const std::string &val) = 0;

        virtual std::string getTypeName() const = 0;

    protected:
        std::string m_name;
        std::string m_description;
    };

    // F: from type, T: to type
    template <class F, class T>
    class LexicalCast
    {
    public:
        T operator()(const F &v)
        {
            return boost::lexical_cast<T>(v);
        }
    };

    template <class T>
    class LexicalCast<std::string, std::vector<T>>
    {
    public:
        std::vector<T> operator()(const std::string &v)
        {
            std::vector<T> vec;
            YAML::Node node = YAML::Load(v);

            for (size_t i = 0; i < node.size(); ++i)
            {
                std::stringstream ss;
                ss << node[i];
                vec.push_back(LexicalCast<std::string, T>()(ss.str()));
            }

            return vec;
        }
    };

    template <class T>
    class LexicalCast<std::vector<T>, std::string>
    {
    public:
        std::string operator()(const std::vector<T> &v)
        {
            YAML::Node node;
            for (auto i : v)
            {
                node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
            }
            std::stringstream ss;
            ss << node;

            return ss.str();
        }
    };

    template <class T>
    class LexicalCast<std::string, std::list<T>>
    {
    public:
        std::list<T> operator()(const std::string &v)
        {
            std::list<T> vec;
            YAML::Node node = YAML::Load(v);

            std::stringstream ss;
            for (size_t i = 0; i < node.size(); ++i)
            {
                ss.str("");
                ss << node[i];
                vec.push_back(LexicalCast<std::string, T>()(ss.str()));
            }

            return vec;
        }
    };

    template <class T>
    class LexicalCast<std::list<T>, std::string>
    {
    public:
        std::string operator()(const std::list<T> &v)
        {
            YAML::Node node;
            for (auto i : v)
            {
                node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
            }
            std::stringstream ss;
            ss << node;

            return ss.str();
        }
    };

    template <class T>
    class LexicalCast<std::string, std::set<T>>
    {
    public:
        std::set<T> operator()(const std::string &v)
        {
            std::set<T> s;
            YAML::Node node = YAML::Load(v);

            std::stringstream ss;
            for (size_t i = 0; i < node.size(); ++i)
            {
                ss.str("");
                ss << node[i];
                s.insert(LexicalCast<std::string, T>()(ss.str()));
            }

            return s;
        }
    };

    template <class T>
    class LexicalCast<std::set<T>, std::string>
    {
    public:
        std::string operator()(const std::set<T> &v)
        {
            YAML::Node node;
            for (auto i : v)
            {
                node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
            }
            std::stringstream ss;
            ss << node;

            return ss.str();
        }
    };

    template <class T>
    class LexicalCast<std::string, std::unordered_set<T>>
    {
    public:
        std::unordered_set<T> operator()(const std::string &v)
        {
            std::unordered_set<T> s;
            YAML::Node node = YAML::Load(v);

            std::stringstream ss;
            for (size_t i = 0; i < node.size(); ++i)
            {
                ss.str("");
                ss << node[i];
                s.insert(LexicalCast<std::string, T>()(ss.str()));
            }

            return s;
        }
    };

    template <class T>
    class LexicalCast<std::unordered_set<T>, std::string>
    {
    public:
        std::string operator()(const std::unordered_set<T> &v)
        {
            YAML::Node node;
            for (auto i : v)
            {
                node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
            }
            std::stringstream ss;
            ss << node;

            return ss.str();
        }
    };

    template <class T>
    class LexicalCast<std::string, std::map<std::string, T>>
    {
    public:
        std::map<std::string, T> operator()(const std::string &v)
        {
            std::map<std::string, T> m;
            YAML::Node node = YAML::Load(v);

            std::stringstream ss;
            for (auto it = node.begin(); it != node.end(); ++it)
            {
                ss.str("");
                ss << it->second;
                m.insert(std::make_pair(it->first.Scalar(), LexicalCast<std::string, T>()(ss.str())));
            }

            return m;
        }
    };

    template <class T>
    class LexicalCast<std::map<std::string, T>, std::string>
    {
    public:
        std::string operator()(const std::map<std::string, T> &v)
        {
            YAML::Node node;
            for (auto i : v)
            {
                node[i.first] = YAML::Load(LexicalCast<T, std::string>()(i.second));
            }
            std::stringstream ss;
            ss << node;

            return ss.str();
        }
    };

    template <class T>
    class LexicalCast<std::string, std::unordered_map<std::string, T>>
    {
    public:
        std::unordered_map<std::string, T> operator()(const std::string &v)
        {
            std::unordered_map<std::string, T> m;
            YAML::Node node = YAML::Load(v);

            std::stringstream ss;
            for (auto it = node.begin(); it != node.end(); ++it)
            {
                ss.str("");
                ss << it->second;
                m.insert(std::make_pair(it->first.Scalar(), LexicalCast<std::string, T>()(ss.str())));
            }

            return m;
        }
    };

    template <class T>
    class LexicalCast<std::unordered_map<std::string, T>, std::string>
    {
    public:
        std::string operator()(const std::unordered_map<std::string, T> &v)
        {
            YAML::Node node;
            for (auto i : v)
            {
                node[i.first] = YAML::Load(LexicalCast<T, std::string>()(i.second));
            }
            std::stringstream ss;
            ss << node;

            return ss.str();
        }
    };

    // FromStr : T operator(const std::string &)
    // ToStr : std::string operator() (const T&)
    template <class T,
              class FromStr = LexicalCast<std::string, T>,
              class ToStr = LexicalCast<T, std::string>>
    class ConfigVar : public ConfigVarBase
    {
    public:
        typedef std::shared_ptr<ConfigVar> ptr;
        typedef std::function<void(const T &old_value, const T &new_value)> on_change_cb;
        typedef RWMutex RWMutexType;

        ConfigVar(const std::string &name, const T &default_value, const std::string &description = "")
            : ConfigVarBase(name, description), m_val(default_value) {}

        std::string toString() override
        {
            try
            {
                // return boost::lexical_cast<std::string>(m_val);
                RWMutexType::ReadLock lock(m_mutex);
                return ToStr()(m_val);
            }
            catch (const std::exception &e)
            {
                LOG_ERROR(LOG_ROOT()) << "ConfigVar::toString exception" << e.what()
                                      << " convert: " << typeid(m_val).name() << " to string";
            }

            return "";
        }
        bool fromString(const std::string &val) override
        {
            try
            {
                // m_val = boost::lexical_cast<T>(val);
                setValue(FromStr()(val));
            }
            catch (std::exception &e)
            {
                LOG_ERROR(LOG_ROOT()) << "ConfigVar::fromString exception" << e.what()
                                      << " convert string to" << typeid(m_val).name();
            }
            return false;
        }

        const T getValue()
        {
            RWMutexType::ReadLock lock(m_mutex);
            return m_val;
        }
        void setValue(const T &val)
        {
            {
                RWMutexType::ReadLock lock(m_mutex);
                if (val == m_val)
                    return;

                for (auto &f : m_cbs)
                {
                    f.second(m_val, val);
                }
            }
            RWMutexType::WriteLock lock(m_mutex);
            m_val = val;
        }
        std::string getTypeName() const override { return typeid(T).name(); }

        uint64_t addListener(on_change_cb cb)
        {
            static uint64_t s_fun_id = 0;
            RWMutexType::WriteLock lock(m_mutex);
            ++s_fun_id;
            m_cbs[s_fun_id] = cb;
            return s_fun_id;
        }

        void delListener(uint64_t key)
        {
            RWMutexType::WriteLock lock(m_mutex);
            m_cbs.erase(key);
        }

        on_change_cb getListener(uint64_t key)
        {
            RWMutexType::ReadLock lock(m_mutex);
            auto it = m_cbs.find(key);
            return it == m_cbs.end() ? nullptr : it->second;
        }

        void clearListener()
        {
            RWMutexType::WriteLock lock(m_mutex);
            m_cbs.clear();
        }

    private:
        T m_val;

        // callback functions when value changes.
        std::map<uint64_t, on_change_cb> m_cbs;
        RWMutexType m_mutex;
    };

    class Config
    {

    public:
        typedef std::map<std::string, ConfigVarBase::ptr> ConfigVarMap;
        typedef RWMutex RWMutexType;

        template <class T>
        static typename ConfigVar<T>::ptr Lookup(const std::string &name, const T &default_value, const std::string &desc = "")
        {
            RWMutexType::WriteLock lock(GetMutex());
            auto &data_map = GetDatas();
            auto it = data_map.find(name);
            if (it != data_map.end())
            {
                auto tmp = std::dynamic_pointer_cast<ConfigVar<T>>(it->second);
                if (tmp)
                {
                    LOG_INFO(LOG_ROOT()) << "Lookup name " << name << " exists";
                    return tmp;
                }
                // if value exists but not the same type, report issue
                LOG_ERROR(LOG_ROOT()) << "Lookup name " << name << " exists but type not " << typeid(T).name()
                                      << " real_type=" << it->second->getTypeName();
            }

            // create new entry
            if (name.find_first_not_of("abcdefghijklmnopqrstuvwxyz._0123456789") != std::string::npos)
            {
                LOG_ERROR(LOG_ROOT()) << "Lookup name invalid " << name;
                throw std::invalid_argument(name);
            }

            typename ConfigVar<T>::ptr v(new ConfigVar<T>(name, default_value, desc));
            data_map[name] = v;
            return v;
        }

        template <class T>
        static typename ConfigVar<T>::ptr Lookup(const std::string &name)
        {
            RWMutexType::ReadLock lock(GetMutex());
            auto &data_map = GetDatas();
            auto it = data_map.find(name);
            if (it == data_map.end())
            {

                return nullptr;
            }

            return std::dynamic_pointer_cast<ConfigVar<T>>(it->second);
        }

        static void LoadFromYaml(const YAML::Node &root);

        static ConfigVarBase::ptr LookupBase(const std::string &name);

        static void Visit(std::function<void(ConfigVarBase::ptr)> cb);

    private:
        static ConfigVarMap &GetDatas()
        {
            static ConfigVarMap s_datas;
            return s_datas;
        }

        static RWMutexType &GetMutex()
        {
            static RWMutexType s_mutex;
            return s_mutex;
        }
    };
}