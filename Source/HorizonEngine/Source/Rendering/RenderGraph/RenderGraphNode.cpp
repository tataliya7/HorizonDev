#include "RenderGraphNode.h"

namespace Horizon
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