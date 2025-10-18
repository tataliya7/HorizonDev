#include "ConfigurationParser.h"

#include <entt/entt.hpp>
#include <toml++/toml.hpp>

namespace Horizon
{
    ConfigurationNode ConfigurationNode::Null = ConfigurationNode(nullptr);

    ConfigurationNode ConfigurationNode::Get(std::string_view key) const
    {
        if (node)
        {
            toml::node* tomlNode = static_cast<toml::node*>(node);
            return tomlNode->at_path(key).node();
        }

        return Null;
    }

    bool ConfigurationNode::AsBooleanOr(bool value) const
    {
        if (node)
        {
            toml::node* tomlNode = static_cast<toml::node*>(node);
            return tomlNode->is_boolean() ? tomlNode->as_boolean()->get() : value;
        }

        return value;
    }

    std::string ConfigurationNode::AsStringOr(const std::string& value) const
    {
        if (node)
        {
            toml::node* tomlNode = static_cast<toml::node*>(node);
            return tomlNode->is_string() ? tomlNode->as_string()->get() : value;
        }

        return value;
    }

    class TomlConfigurationTable : public ConfigurationTable
    {
    public:

        TomlConfigurationTable(const std::filesystem::path& path)
        {
            table = toml::parse_file(path.string());
        }

        ConfigurationNode Get(std::string_view key) override
        {
            return table.at_path(key).node();
        }

    private:

        toml::table table;
    };


    ConfigurationParser ConfigurationParser::ParseFile(const std::filesystem::path& path)
    {
        ConfigurationTable* table = new TomlConfigurationTable(path);
        return ConfigurationParser(table);
    }

    ConfigurationParser::ConfigurationParser(ConfigurationTable* table)
        : table(table)
    {

    }

    ConfigurationParser::~ConfigurationParser()
    {
        delete table;
    }

    ConfigurationNode ConfigurationParser::Get(std::string_view key)
    {
        if (table)
        {
            return table->Get(key);
        }

        return ConfigurationNode::Null;
    }
}