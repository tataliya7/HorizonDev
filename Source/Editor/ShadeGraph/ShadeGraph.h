#pragma once

#include <HorizonEngine.h>

namespace HE
{
    class ShadeGraph
    {
    public:
    };

    class ShadeGraphPin
    {
    public:
        std::string name;
        std::string description;
    };

    class ShadeGraphFloatPin : public ShadeGraphPin
    {
    protected:

        float defaultValue = 0.0f;
        float minValue = std::numeric_limits<float>::min();
        float maxValue = std::numeric_limits<float>::max();

    public:
        ShadeGraphFloatPin& SetDefaultValue(float value)
        {
            defaultValue = value;
            return *this;
        }

        ShadeGraphFloatPin& SetMinValue(float value)
        {
            minValue = value;
            return *this;
        }

        ShadeGraphFloatPin& SetMaxValue(float value)
        {
            maxValue = value;
            return *this;
        }
    };

    class ShadeGraphColorPin : public ShadeGraphPin
    {
    protected:

        float defaultValue[4] = { 1.0f, 1.0f, 1.0f, 1.0f };

    public:

        ShadeGraphColorPin& SetDefaultValue(float r, float g, float b, float a)
        {
            defaultValue[0] = r;
            defaultValue[1] = g;
            defaultValue[2] = b;
            defaultValue[3] = a;
            return *this;
        }
    };

    class ShadeGraphNodeType
    {
    public:
        std::string uniqueName;
        std::vector<std::unique_ptr<ShadeGraphPin>> inputs;
        std::vector<std::unique_ptr<ShadeGraphPin>> outputs;

        float width;
        float height; // Height is calculate automatically for most nodes.

        void (*DeclarePins)(ShadeGraphNodeType& nodeType);

        template<typename PinType>
        inline PinType& AddInput(const std::string& name)
        {
            PinType* pin = new PinType();
            pin->name = name;
            inputs.push_back(std::unique_ptr<PinType>(pin));
            return *pin;
        }

        template<typename PinType>
        inline PinType& AddOutput(const std::string& name)
        {
            PinType* pin = new PinType();
            pin->name = name;
            outputs.push_back(std::unique_ptr<PinType>(pin));
            return *pin;
        }
    };

    enum class ShadeGraphPinType
    {
        Float,
    };

    class ShadeGraphPinInstance
    {
    public:
        ShadeGraphPin* type;

        uint32 GetIndexInNode() const
        {
            return indexInNode;
        }

        uint32 indexInNode;

        bool IsInput() const
        {
            return true;
        }

        bool IsOutput() const
        {
            return true;
        }

        int IndexInAllInputs() const
        {
            return 0;
        }
    };

    class ShadeGraphNode
    {
    public:

        ShadeGraphNodeType* type;
        int id;
        std::string title;

        void Init();

        std::vector<ShadeGraphPinInstance*> inputs;
        std::vector<ShadeGraphPinInstance*> outputs;
    };

    /*enum class ShadeGraph
    {

    };

    struct ShadeGraphNodeSampleTexture2D
    {
        NodeTexBase base;
        int interpolation;
    };*/

    void ShaderGraphSystemInit();
    void ShadeGraphSystemExit();

    void ShadeGraphRegisterNodeType(ShadeGraphNodeType* nodeType);
    ShadeGraphNodeType* ShadeGraphFindNodeType(const char* name);

    void ShadeGraphRegisterNodeTypeMultiply();
    void ShadeGraphRegisterNodeTypeSampleTexture2D();
    void ShadeGraphRegisterNodeTypePrincipledBSDF();
}