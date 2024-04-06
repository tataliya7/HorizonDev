#include "Rendering/RenderGraph/RenderGraphNode.h"

namespace HE
{
    void RenderGraphDAG::RegisterNode(RenderGraphNode* node)
    {
        nodes.push_back(node);
    }

    void RenderGraphDAG::Clear()
    {
        nodes.clear();
    }
}