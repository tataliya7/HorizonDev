#pragma once

#include "Foundation/FoundationModule.h"

namespace Horizon
{
    class ConfigurationParser;

    class ConfigurationNode
    {
    public:

        static ConfigurationNode Null;

        ConfigurationNode(void* node)
            : node(node)
        {

        }

        virtual ~ConfigurationNode() = default;

        ConfigurationNode Get(std::string_view key) const;

        bool AsBooleanOr(bool value) const;

        std::string AsStringOr(const std::string& value) const;

    protected:

        void* node;
    };

    class ConfigurationTable
    {
    public:

        ConfigurationTable() = default;

        virtual ~ConfigurationTable() = default;

        virtual ConfigurationNode Get(std::string_view key)
        {
            return ConfigurationNode::Null;
        }
    };

    class ConfigurationParser
    {
    public:

        static ConfigurationParser ParseFile(const std::filesystem::path& path);

        ~ConfigurationParser();

        ConfigurationNode Get(std::string_view key);

    protected:

        ConfigurationParser(ConfigurationTable* table);

        ConfigurationTable* table;
    };
}