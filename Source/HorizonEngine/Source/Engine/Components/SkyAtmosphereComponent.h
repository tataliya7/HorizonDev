#pragma once

#include "Foundation/FoundationModule.h"
#include "Engine/Serialization/Archive.h"

namespace Horizon
{
    class SkyAtmosphereRenderObject;

    struct DensityProfileLayer
    {
        float width;
        float expTerm;
        float expScale;
        float linearTerm;
        float constantTerm;
    };

    class SkyAtmosphereComponent
    {
    public:

        SkyAtmosphereComponent();

        ~SkyAtmosphereComponent();

        /** The distance (kilometers) between the planet center and the ground. */
        float groundRadius;

        /** The average albedo of the ground. */
        Vector3 groundAlbedo;

        /** The distance (kilometers) between the ground and the top of the atmosphere. */
        float atmosphereHeight;

        /** Rayleigh scattering scaling factor.*/
        float rayleighScatteringScale;

        /** The scattering coefficient of air molecules at the altitude where their density is maximum (usually the bottom of the atmosphere). */
        Vector3 rayleighScattering;

        /** TBD.*/
        float rayleighExponentialDistribution;

        /** Mie scattering scaling factor.*/
        float mieScatteringScale;

        /** The scattering coefficient of aerosols at the altitude where their density is maximum (usually the bottom of the atmosphere). */
        Vector3 mieScattering;

        /** TBD.*/
        float mieAbsorptionScale;

        /** TBD.*/
        Vector3 mieAbsorption;

        /** The asymmetry parameter for the Henyey Greenstein Phase Function. The value of this parameter must be in the range (-1, 1). */
        float mieAsymmetry;

        /** TBD.*/
        float mieExponentialDistribution;

        /** TBD.*/
        DensityProfileLayer absorptionDensity[2];

        /** TBD.*/
        float absorptionExtinctionScale;

        /**
         * The extinction coefficient of molecules that absorb light (e.g. ozone) at the altitude where their density is maximum.
         *
         * @details The extinction coefficient at altitude h is equal to 'absorption extinction' times 'absorption density' at this altitude.
         */
        Vector3 absorptionExtinction;

        /// The cosine of the maximum Sun zenith angle for which atmospheric scattering
        /// must be precomputed (for maximum precision, use the smallest Sun zenith
        /// angle yielding negligible sky light radiance values. For instance, for the
        /// Earth case, 102 degrees is a good choice - yielding mu_s_min = -0.2).
        //float cosMaxSunZenithAngle;

        /** TBD.*/
        float skyLuminanceScale;

        /** TBD.*/
        Vector3 skyLuminanceColor;

        void Serialize(Archive& archive);

        void UpdateRenderObject();

    private:

        SkyAtmosphereRenderObject* renderObject;
    };
}