#include "ShadeGraph.h"

namespace HE
{
    static std::map<std::string, ShadeGraphNodeType*> GShadeGraphNodeTypeMap;

    void ShadeGraphRegisterNodeType(ShadeGraphNodeType* nodeType)
    {
        GShadeGraphNodeTypeMap.emplace(nodeType->uniqueName, nodeType);
    }

    ShadeGraphNodeType* ShadeGraphFindNodeType(const char* name)
    {
        if (GShadeGraphNodeTypeMap.find(name) != GShadeGraphNodeTypeMap.end())
        {
            return GShadeGraphNodeTypeMap[name];
        }
        return nullptr;
    }

    void ShaderGraphSystemInit()
    {
        ShadeGraphRegisterNodeTypeMultiply();
        ShadeGraphRegisterNodeTypeSampleTexture2D();
        ShadeGraphRegisterNodeTypePrincipledBSDF();
    }

    void ShadeGraphSystemExit()
    {

    }

    void ShadeGraphNode::Init()
    {
        for (uint32 i = 0; i < type->inputs.size(); i++)
        {
            ShadeGraphPin* pin = type->inputs[i].get();
            auto pinInstance = new ShadeGraphPinInstance();
            pinInstance->type = pin;
            pinInstance->indexInNode = (uint32)inputs.size();
            inputs.push_back(pinInstance);
        }

        for (uint32 i = 0; i < type->outputs.size(); i++)
        {
            ShadeGraphPin* pin = type->outputs[i].get();
            auto pinInstance = new ShadeGraphPinInstance();
            pinInstance->type = pin;
            pinInstance->indexInNode = (uint32)outputs.size();
            outputs.push_back(pinInstance);
        }
    }
}
