#include "SkyAtmosphereComponent.h"
#include "Rendering/RenderingModule.h"

namespace Horizon
{
    static void SetupAtmosphereOfEarth(SkyAtmosphereComponent* component)
    {
        // Default settings are parameters of the Earth's atmosphere.
        // Values shown here are the result of integration over wavelength power spectrum integrated with paricular function.
        // Refer to https://github.com/ebruneton/precomputed_atmospheric_scattering for details.
        // All units in kilometers.

        constexpr float earthRadius = 6360.0f;
        constexpr float earthAtmosphereHeight = 60.0f;
        //const float earthAtmosphereHeight = 100.0f; // 100km atmosphere radius, less edge visible and it contain 99.99% of the atmosphere medium https://en.wikipedia.org/wiki/K%C3%A1rm%C3%A1n_line
        constexpr float earthRayleighScaleHeight = 8.0f;
        constexpr float earthMieScaleHeight = 1.2f;

        //const double maxSunZenithAngle = M_PI * 120.0 / 180.0;

        // Ground
        component->groundRadius = earthRadius;
        component->groundAlbedo = Vector3(0.401978f, 0.401978f, 0.401978f);

        // Atmosphere Height
        component->atmosphereHeight = earthAtmosphereHeight;

        // Raleigh
        constexpr Vector3 earthRayleighScattering = Vector3(0.005802f, 0.013558f, 0.033100f);
        component->rayleighScatteringScale = std::max(std::max(earthRayleighScattering.r, earthRayleighScattering.g), earthRayleighScattering.b);
        component->rayleighScattering = earthRayleighScattering / component->rayleighScatteringScale;
        component->rayleighExponentialDistribution = earthRayleighScaleHeight;

        // Mie
        constexpr Vector3 earthMieScattering = Vector3(0.003996f, 0.003996f, 0.003996f);
        constexpr Vector3 earthMieExtinction = Vector3(0.004440f, 0.004440f, 0.004440f);
        constexpr Vector3 earthMieAbsorption = earthMieExtinction - earthMieScattering;
        component->mieScatteringScale = std::max(std::max(earthMieScattering.r, earthMieScattering.g), earthMieScattering.b);
        component->mieScattering = earthMieScattering / component->mieScatteringScale;
        component->mieAbsorptionScale = std::max(std::max(earthMieAbsorption.r, earthMieAbsorption.g), earthMieAbsorption.b);
        component->mieAbsorption = earthMieAbsorption / component->mieAbsorptionScale;
        component->mieAsymmetry = 0.8f;
        component->mieExponentialDistribution = earthMieScaleHeight;

        // Ozone
        constexpr Vector3 earthAbsorptionExtinction = Vector3(0.000650f, 0.001881f, 0.000085f);
        component->absorptionDensity[0] = { 25.0f, 0.0f, 0.0f, 1.0f / 15.0f, -2.0f / 3.0f };
        component->absorptionDensity[1] = { 0.0f, 0.0f, 0.0f, -1.0f / 15.0f, 8.0f / 3.0f };
        component->absorptionExtinctionScale = std::max(std::max(earthAbsorptionExtinction.r, earthAbsorptionExtinction.g), earthAbsorptionExtinction.b);
        component->absorptionExtinction = earthAbsorptionExtinction / component->absorptionExtinctionScale;
    }

    SkyAtmosphereComponent::SkyAtmosphereComponent()
    {
        SetupAtmosphereOfEarth(this);

        skyLuminanceScale = 1.0f;
        skyLuminanceColor = Vector3(1.0f, 1.0f, 1.0f);

        renderObject = nullptr;
    }

    SkyAtmosphereComponent::~SkyAtmosphereComponent()
    {
        assert(renderObject == nullptr);
    }

    void SkyAtmosphereComponent::Serialize(Archive& archive)
    {

    }

    static void SetupAtmosphereParameters(AtmosphereParameters& outAtmosphereParameters, const SkyAtmosphereComponent& skyAtmosphereComponent)
    {
        outAtmosphereParameters.bottomRadius = skyAtmosphereComponent.groundRadius;
        outAtmosphereParameters.topRadius = skyAtmosphereComponent.groundRadius + skyAtmosphereComponent.atmosphereHeight;
        outAtmosphereParameters.groundAlbedo = skyAtmosphereComponent.groundAlbedo; // TODO: sRGB to linear color
        outAtmosphereParameters.rayleighScattering = skyAtmosphereComponent.rayleighScatteringScale * skyAtmosphereComponent.rayleighScattering;
        outAtmosphereParameters.rayleighDensityExpScale = -1.0f / skyAtmosphereComponent.rayleighExponentialDistribution;
        outAtmosphereParameters.mieScattering = skyAtmosphereComponent.mieScatteringScale * skyAtmosphereComponent.mieScattering;
        outAtmosphereParameters.mieAbsorption = skyAtmosphereComponent.mieAbsorptionScale * skyAtmosphereComponent.mieAbsorption;
        outAtmosphereParameters.mieExtinction = outAtmosphereParameters.mieScattering + outAtmosphereParameters.mieAbsorption;
        outAtmosphereParameters.miePhaseG = std::clamp(skyAtmosphereComponent.mieAsymmetry, -1.0f, 1.0f);
        outAtmosphereParameters.mieDensityExpScale = -1.0f / skyAtmosphereComponent.mieExponentialDistribution;
        outAtmosphereParameters.absorptionDensity0LayerWidth = skyAtmosphereComponent.absorptionDensity[0].width;
        outAtmosphereParameters.absorptionDensity0ConstantTerm = skyAtmosphereComponent.absorptionDensity[0].constantTerm;
        outAtmosphereParameters.absorptionDensity0LinearTerm = skyAtmosphereComponent.absorptionDensity[0].linearTerm;
        outAtmosphereParameters.absorptionDensity1ConstantTerm = skyAtmosphereComponent.absorptionDensity[0].constantTerm;
        outAtmosphereParameters.absorptionDensity1LinearTerm = skyAtmosphereComponent.absorptionDensity[0].linearTerm;
        outAtmosphereParameters.absorptionExtinction = skyAtmosphereComponent.absorptionExtinctionScale * skyAtmosphereComponent.absorptionExtinction;
    }

    void SkyAtmosphereComponent::UpdateRenderObject()
    {
        if (renderObject)
        {
            AtmosphereParameters atmosphereParameters = {};
            SetupAtmosphereParameters(atmosphereParameters, *this);
            renderObject->SetAtmosphereParameters(atmosphereParameters);

            Vector3 skyLuminanceFactor = skyLuminanceScale * skyLuminanceColor;
            renderObject->SetSkyLuminanceFactor(skyLuminanceFactor);
        }
    }
}