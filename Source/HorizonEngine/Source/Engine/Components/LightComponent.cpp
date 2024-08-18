#include "LightComponent.h"
#include "Rendering/RenderingModule.h"

namespace Horizon
{
    void LightComponent::CreateRenderObject(RenderScene* scene)
    {
        assert(renderObject == nullptr);
        if (renderObject == nullptr)
        {
            LightRenderObjectDescription description;
            description.color = GetPhysicalLightColor();
            description.position = Vector3(0.0f, 0.0f, 0.0f);
            description.direction = forwardVec;
            description.radius = radius;
            description.castRayTracingShadows = castShadows;
            description.usedAsAtmosphericLight = usedAsAtmosphericLight;
            description.halfApexAngleInRadians = GetHalfApexAngleInRadians();
            description.atmosphericLightDiskColorFactor = atmosphericLightDiskColorFactor;
            description.shadowMapSize = shadowMapSize;
            description.shadowCascadeCount = shadowCascadeCount;
            description.shadowCascadeSplitLambda = shadowCascadeSplitLambda;
            description.maxShadowDistance = maxShadowDistance;
            description.shadowMapDepthBiasConstantFactor = shadowMapDepthBiasConstantFactor;
            description.shadowMapDepthBiasSlopeFactor = shadowMapDepthBiasSlopeFactor;

            if (type == Type::Distant)
            {
                description.lightType = LightType::DistantLight;
            }
            else if (type == Type::Point)
            {
                description.lightType = LightType::PointLight;
            }
            else if (type == Type::Spot)
            {
                description.lightType = LightType::SpotLight;
            }

            renderObject = new LightRenderObject(description);
            scene->AddLight(renderObject);
        }
    }

    void LightComponent::DestroyRenderObject(RenderScene* scene)
    {

    }

    void LightComponent::UpdateRenderObject()
    {
        if (renderObject)
        {

        }
    }
}