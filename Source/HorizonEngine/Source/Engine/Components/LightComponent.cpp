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
            description.position = position;
            description.direction = direction;
            description.radius = radius;
            description.castDynamicShadows = castDynamicShadows;
            description.usedAsAtmosphericLight = usedAsAtmosphericLight;
            description.halfApexAngleInRadians = GetHalfApexAngleInRadians();
            description.atmosphericLightDiskColorFactor = atmosphericLightDiskColorFactor;
            description.shadowMapSize = shadowMapSize;
            description.shadowCascadeCount = shadowCascadeCount;
            description.shadowCascadeSplitLambda = shadowCascadeSplitLambda;
            description.shadowCascadeTransitionScale = shadowCascadeTransitionScale;
            description.maxShadowDistance = maxShadowDistance;
            description.shadowFadeOutFactor = shadowFadeOutFactor;
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
            renderObject->color = GetPhysicalLightColor();
            renderObject->position = position;
            renderObject->direction = direction;
            renderObject->radius = radius;
            renderObject->castDynamicShadows = castDynamicShadows;
            renderObject->usedAsAtmosphericLight = usedAsAtmosphericLight;
            renderObject->halfApexAngleInRadians = GetHalfApexAngleInRadians();
            renderObject->atmosphericLightDiskColorFactor = atmosphericLightDiskColorFactor;
            renderObject->shadowMapSize = shadowMapSize;
            renderObject->shadowCascadeCount = shadowCascadeCount;
            renderObject->shadowCascadeSplitLambda = shadowCascadeSplitLambda;
            renderObject->shadowCascadeTransitionScale = shadowCascadeTransitionScale;
            renderObject->maxShadowDistance = maxShadowDistance;
            renderObject->shadowFadeOutFactor = shadowFadeOutFactor;
            renderObject->shadowMapDepthBiasConstantFactor = shadowMapDepthBiasConstantFactor;
            renderObject->shadowMapDepthBiasSlopeFactor = shadowMapDepthBiasSlopeFactor;

            if (type == Type::Distant)
            {
                renderObject->lightType = LightType::DistantLight;
            }
            else if (type == Type::Point)
            {
                renderObject->lightType = LightType::PointLight;
            }
            else if (type == Type::Spot)
            {
                renderObject->lightType = LightType::SpotLight;
            }
        }
    }
}