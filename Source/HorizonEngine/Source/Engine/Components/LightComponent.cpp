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
            description.shadowCascadeTransitionScale = shadowCascadeBlendScale;
            description.maxShadowDistance = maxShadowDistance;
            description.shadowFadeOutFactor = shadowFadeOutFactor;
            description.shadowMapDepthBiasConstantFactor = shadowMapDepthBiasConstantFactor;
            description.shadowMapDepthBiasSlopeFactor = shadowMapDepthBiasSlopeFactor;
            description.enableScreenSpaceShadows = enableScreenSpaceShadows;
            description.screenSpaceShadowsSurfaceThickness = screenSpaceShadowsSurfaceThickness;
            description.screenSpaceShadowsShadowContrast = screenSpaceShadowsShadowContrast;
            description.enableLightShafts = enableScreenSpaceLightShafts;
            description.lightShaftsIntensity = lightShaftsIntensity;
            description.lightShaftsColor = lightShaftsColor;

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
            renderObject->shadowCascadeBlendScale = shadowCascadeBlendScale;
            renderObject->maxShadowDistance = maxShadowDistance;
            renderObject->shadowFadeOutFactor = shadowFadeOutFactor;
            renderObject->shadowMapDepthBiasConstantFactor = shadowMapDepthBiasConstantFactor;
            renderObject->shadowMapDepthBiasSlopeFactor = shadowMapDepthBiasSlopeFactor;
            renderObject->enableScreenSpaceShadows = enableScreenSpaceShadows;
            renderObject->screenSpaceShadowsSurfaceThickness = screenSpaceShadowsSurfaceThickness;
            renderObject->screenSpaceShadowsShadowContrast = screenSpaceShadowsShadowContrast;
            renderObject->enableLightShafts = enableScreenSpaceLightShafts;
            renderObject->lightShaftsIntensity = lightShaftsIntensity;
            renderObject->lightShaftsColor = lightShaftsColor;

            if (type == Type::Distant)
            {
                renderObject->lightType = LightType::DistantLight;
                renderObject->worldToLight = glm::lookAt(Vector3f(0.0f, 0.0f, 0.0f), direction, Vector3f(0.0f, 1.0f, 0.0f));
            }
            else if (type == Type::Point)
            {
                renderObject->lightType = LightType::PointLight;
                renderObject->worldToLight = glm::translate(position);
            }
            else if (type == Type::Spot)
            {
                renderObject->lightType = LightType::SpotLight;
                renderObject->worldToLight = glm::lookAt(position, position + direction, Vector3f(0.0f, 1.0f, 0.0f));
            }
        }
    }
}