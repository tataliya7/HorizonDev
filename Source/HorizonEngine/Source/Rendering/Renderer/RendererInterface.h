#include "RendererCommon.h"

namespace Horizon
{
    /**
     * The renderer implements the process of generating visual images.
     */
    class SceneRenderer
    {
    public:
        virtual void Render(RenderGraph& renderGraph) = 0;
    };
}