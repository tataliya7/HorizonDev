#pragma once

#include "Foundation/FoundationModule.h"
#include "Rendering/RenderingModule.h"

namespace FidelityFX
{
    class FidelityFXSuperResolution2 : public TemporalSuperSamplingInterface
    {
    public:
        AddPass() override;
    private:

    };
}