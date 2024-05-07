#pragma once

#include "Core/CoreModule.h"

namespace Horizon
{
    class LightRenderProxy
    {
    public:
        LightRenderProxy();
        virtual ~LightRenderProxy();
    private:
    };

    class DirectionalLightRenderProxy : public LightRenderProxy
    {
    public:
        DirectionalLightRenderProxy();
        virtual ~DirectionalLightRenderProxy();
    private:
    };

    class SpotLightRenderProxy : public LightRenderProxy
    {
    public:
        SpotLightRenderProxy();
        virtual ~SpotLightRenderProxy();
    private:
    };

    class PointLightRenderProxy : public LightRenderProxy
    {
    public:
        PointLightRenderProxy();
        virtual ~PointLightRenderProxy();
    private:
    };

    class EnvironmentLight
    struct AtmosphereParameters
    {
        float bottomRadius;
        float topRadius;
        Vector3 groundAlbedo;

        Vector3 rayleighScattering;
        float rayleighDensityExpScale;

        Vector3 mieScattering;
        Vector3 mieExtinction;
        Vector3 mieAbsorption;
        float miePhaseG;
        float mieDensityExpScale;

        float absorptionDensity0LayerWidth;
        float absorptionDensity0ConstantTerm;
        float absorptionDensity0LinearTerm;
        float absorptionDensity1ConstantTerm;
        float absorptionDensity1LinearTerm;
        Vector3 absorptionExtinction;
    };

    class SkyAtmosphereRenderProxy
    {
    public:
        SkyAtmosphereRenderProxy();

        const AtmosphereParameters& GetAtmosphereParameters() const
        {

        }

        void SetAtmosphereParameters(const AtmosphereParameters& newValue)
        {
            atmosphereParameters = newValue;
        }

        Vector3 GetSkyLuminanceFactor() const
        {
            return skyLuminanceFactor;
        }

        void SetSkyLuminanceFactor(const Vector3& newValue)
        {
            skyLuminanceFactor = newValue;
        }

    private:
        AtmosphereParameters atmosphereParameters;
        Vector3 skyLuminanceFactor;
    };

    class RenderSceneInterface
    {
    public:

        /**
         * Release this scene.
         */
        virtual void Release() = 0;

        /**
         * Adds a mesh component to the scene.
         *
         * @param[in] component - mesh component to add to the scene.
         */
	    virtual void AddMesh(MeshComponent* component) = 0;

        /**
         * Removes a mesh component from the scene.
         *
         * @param[in] component - mesh component to remove from the scene.
         */
	    virtual void RemoveMesh(MeshComponent* component) = 0;

        /**
         * Adds a new light to the scene.
         *
         * @param [in] light - light to add to the scene.
         */
        virtual void AddLight(LightRenderProxy* light) = 0;

        /**
         * Removes a light component from the scene.
         *
         * @param [in] component light component to remove from the scene.
         */
        virtual void RemoveLight(LightComponent* component) = 0;

        virtual void HasSkyLight() = 0;

        virtual void SetSkyLight(FSkyLightSceneProxy* component) = 0;

        virtual void HasSkyAtmosphere() = 0;

        virtual void SetSkyAtmosphere(SkyAtmosphereRenderProxy* skyAtmosphere) = 0;
    };
}
