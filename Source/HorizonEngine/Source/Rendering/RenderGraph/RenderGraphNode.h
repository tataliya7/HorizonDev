#pragma once

#include "Rendering/RenderGraph/RenderGraphCommon.h"

namespace Horizon
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
            referenceCount = InfiniteReferenceCount;
        }
        bool IsCulled() const
        {
            return referenceCount == 0;
        }
        char const* GetName() const
        {
            return name.c_str();
        }
        uint32 GetReferenceCount() const
        {
            return referenceCount;
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
        static const uint32 InfiniteReferenceCount = (uint32)-1;
        friend class RenderGraph;
        friend class RenderGraphDAG;
        friend class RenderGraphBuilder;
        const std::string name;
        RenderGraphNodeType type;
        uint32 referenceCount = 0;
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
