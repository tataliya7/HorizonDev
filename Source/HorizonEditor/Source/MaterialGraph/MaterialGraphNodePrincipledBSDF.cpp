#include "ShadeGraph.h"

namespace Horizon::Nodes::PrincipledBSDF
{
    static void DeclarePins(ShadeGraphNodeType& node)
    {
        node.AddInput<ShadeGraphColorPin>("Base Color")
            .SetDefaultValue(1.0f, 1.0f, 1.0f, 1.0f);
        node.AddInput<ShadeGraphFloatPin>("Subsurface")
            .SetDefaultValue(0.0f)
            .SetMinValue(0.0f)
            .SetMaxValue(1.0f);
        node.AddInput<ShadeGraphFloatPin>("Metallic")
            .SetDefaultValue(0.0f)
            .SetMinValue(0.0f)
            .SetMaxValue(1.0f);
        node.AddInput<ShadeGraphFloatPin>("Specular")
            .SetDefaultValue(0.5f)
            .SetMinValue(0.0f)
            .SetMaxValue(1.0f);
        node.AddInput<ShadeGraphFloatPin>("Specular Tint")
            .SetDefaultValue(0.0f)
            .SetMinValue(0.0f)
            .SetMaxValue(1.0f);
        node.AddInput<ShadeGraphFloatPin>("Roughness")
            .SetDefaultValue(0.0f)
            .SetMinValue(0.0f)
            .SetMaxValue(1.0f);
        node.AddInput<ShadeGraphFloatPin>("Anisotropic")
            .SetDefaultValue(0.0f)
            .SetMinValue(0.0f)
            .SetMaxValue(1.0f);
        node.AddInput<ShadeGraphFloatPin>("Sheen")
            .SetDefaultValue(0.0f)
            .SetMinValue(0.0f)
            .SetMaxValue(1.0f);
        node.AddInput<ShadeGraphFloatPin>("Sheen Tint")
            .SetDefaultValue(0.0f)
            .SetMinValue(0.0f)
            .SetMaxValue(1.0f);
        node.AddInput<ShadeGraphFloatPin>("Clearcoat")
            .SetDefaultValue(0.0f)
            .SetMinValue(0.0f)
            .SetMaxValue(1.0f);
        node.AddInput<ShadeGraphFloatPin>("Clearcoat Roughness")
            .SetDefaultValue(0.03f)
            .SetMinValue(0.0f)
            .SetMaxValue(1.0f);
        node.AddInput<ShadeGraphFloatPin>("IOR")
            .SetDefaultValue(1.5f)
            .SetMinValue(0.0f)
            .SetMaxValue(1000.0f);
        node.AddInput<ShadeGraphFloatPin>("Transmission")
            .SetDefaultValue(0.0f)
            .SetMinValue(0.0f)
            .SetMaxValue(1.0f);
        node.AddInput<ShadeGraphFloatPin>("Emission Strength")
            .SetDefaultValue(0.0f)
            .SetMinValue(0.0f)
            .SetMaxValue(1.0f);
        node.AddInput<ShadeGraphFloatPin>("Alpha")
            .SetDefaultValue(0.0f)
            .SetMinValue(0.0f)
            .SetMaxValue(1.0f);
    }
}

namespace Horizon
{
    void ShadeGraphRegisterNodeTypePrincipledBSDF()
    {
        static ShadeGraphNodeType nodeType;

        nodeType.uniqueName = "Principled BSDF";
        nodeType.DeclarePins = HE::Nodes::PrincipledBSDF::DeclarePins;

        nodeType.DeclarePins(nodeType);
        ShadeGraphRegisterNodeType(&nodeType);
    }
}

namespace Horizon::Nodes::Multiply
{
    static void DeclarePins(ShadeGraphNodeType& node)
    {
        node.AddInput<ShadeGraphColorPin>("Base Color")
            .SetDefaultValue(1.0f, 1.0f, 1.0f, 1.0f);
        node.AddInput<ShadeGraphFloatPin>("Subsurface")
            .SetDefaultValue(0.0f)
            .SetMinValue(0.0f)
            .SetMaxValue(1.0f);
    }
}

namespace Horizon
{
    void ShadeGraphRegisterNodeTypeMultiply()
    {
        static ShadeGraphNodeType nodeType;

        nodeType.uniqueName = "Multiply";
        nodeType.DeclarePins = HE::Nodes::Multiply::DeclarePins;

        nodeType.DeclarePins(nodeType);
        ShadeGraphRegisterNodeType(&nodeType);
    }
}

namespace Horizon::Nodes::SampleTexture2D
{
    static void DeclarePins(ShadeGraphNodeType& node)
    {
        node.AddOutput<ShadeGraphColorPin>("Color");
        node.AddOutput<ShadeGraphFloatPin>("Alpha");
    }
}

namespace Horizon
{
    void ShadeGraphRegisterNodeTypeSampleTexture2D()
    {
        static ShadeGraphNodeType nodeType;

        nodeType.uniqueName = "Sample Texture 2D";
        nodeType.DeclarePins = HE::Nodes::SampleTexture2D::DeclarePins;

        nodeType.DeclarePins(nodeType);
        ShadeGraphRegisterNodeType(&nodeType);
    }
}