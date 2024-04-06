#pragma once

#include "Rendering/RenderGraph/RenderGraphCommon.h"

namespace HE
{
    enum class RenderGraphNodeType
    {
        Pass,
        Resource,
    };

    class RenderGraphNode
    {
    public:
        RenderGraphNode(const std::string& name, RenderGraphNodeType type)
            : name(name), type(type) {}
        virtual ~RenderGraphNode() = default;
        void NeverCull()
        {
            refCount = InfRefCount;
        }
        bool IsCulled() const
        {
            return refCount == 0;
        }
        char const* GetName() const
        {
            return name.c_str();
        }
        uint32 GetRefCount() const
        {
            return refCount;
        }
        const std::vector<RenderGraphNode*>& GetInputs() const
        {
            return inputs;
        }
        const std::vector<RenderGraphNode*>& GetOutputs() const
        {
            return outputs;
        }
    protected:
        friend class RenderGraph;
        friend class RenderGraphDAG;
        friend class RenderGraphBuilder;
        const std::string name;
        RenderGraphNodeType type;
        uint32 refCount = 0;
        static const uint32 InfRefCount = (uint32)-1;
        std::vector<RenderGraphNode*> inputs;
        std::vector<RenderGraphNode*> outputs;
    };

    class RenderGraphDAG
    {
    public:
        RenderGraphDAG() = default;
        ~RenderGraphDAG() = default;
        void RegisterNode(RenderGraphNode* node);
        void Clear();
    private:
        friend class RenderGraph;
        std::vector<RenderGraphNode*> nodes;
    };
}
