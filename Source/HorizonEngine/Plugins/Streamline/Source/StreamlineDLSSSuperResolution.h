#pragma once

#include "Foundation/FoundationModule.h"
#include "Rendering/RenderingModule.h"

namespace Streamline
{
    class StreamlineDLSS : public TemporalSuperSamplingInterface
    {
    public:
        AddPass() override;
    private:

    };
}