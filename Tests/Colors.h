#pragma once

#include <gtest/gtest.h>

#include "Core/CoreModule.h"

namespace HE
{
    glm::f32mat3x3 calc_sat_adjust_matrix(float sat, float rgb2Y[3])
    {
        //
        // This function determines the terms for a 3x3 saturation matrix that is
        // based on the luminance of the input.
        //
        glm::f32mat3x3 M;
        M[0][0] = (1.0f - sat) * rgb2Y[0] + sat;
        M[1][0] = (1.0f - sat) * rgb2Y[0];
        M[2][0] = (1.0f - sat) * rgb2Y[0];

        M[0][1] = (1.0f - sat) * rgb2Y[1];
        M[1][1] = (1.0f - sat) * rgb2Y[1] + sat;
        M[2][1] = (1.0f - sat) * rgb2Y[1];

        M[0][2] = (1.0f - sat) * rgb2Y[2];
        M[1][2] = (1.0f - sat) * rgb2Y[2];
        M[2][2] = (1.0f - sat) * rgb2Y[2] + sat;

        return M;
    }

    glm::f64mat3x3 calc_sat_adjust_matrix(double sat, double rgb2Y[3])
    {
        //
        // This function determines the terms for a 3x3 saturation matrix that is
        // based on the luminance of the input.
        //
        glm::f64mat3x3 M;
        M[0][0] = (1.0 - sat) * rgb2Y[0] + sat;
        M[1][0] = (1.0 - sat) * rgb2Y[0];
        M[2][0] = (1.0 - sat) * rgb2Y[0];

        M[0][1] = (1.0 - sat) * rgb2Y[1];
        M[1][1] = (1.0 - sat) * rgb2Y[1] + sat;
        M[2][1] = (1.0 - sat) * rgb2Y[1];

        M[0][2] = (1.0 - sat) * rgb2Y[2];
        M[1][2] = (1.0 - sat) * rgb2Y[2];
        M[2][2] = (1.0 - sat) * rgb2Y[2] + sat;

        return M;
    }

    TEST(ColorTest, Run)
    {
        const Matrix3x3 m1 = Matrix3x3(
            0.31670331f, 0.70299344f, 0.08120592f,
            0.10129085f, 0.72118661f, 0.12041039f,
            0.01451538f, 0.05643031f, 0.53416779f);

        const Matrix3x3 m2 = Matrix3x3(
            4.57829597f, -4.48749114f, 0.31554848f,
            -0.63342362f, 2.03236026f, -0.36183302f,
            -0.05749394f, -0.09275939f, 1.90172089f);

        auto ccc = m1 * m2;

        // "Glow" module constants
        const float RRT_GLOW_GAIN = 0.05f;
        const float RRT_GLOW_MID = 0.08f;

        // Red modifier constants
        const float RRT_RED_SCALE = 0.82f;
        const float RRT_RED_PIVOT = 0.03f;
        const float RRT_RED_HUE = 0.0f;
        const float RRT_RED_WIDTH = 135.0f;

        float AP1_RGB2Y[3] = { 0.2722287168f,  0.6740817658f,  0.0536895174f };

        // Desaturation contants
        const float RRT_SAT_FACTOR = 0.96f;
        const glm::f32mat3x3 RRT_SAT_MAT = calc_sat_adjust_matrix(RRT_SAT_FACTOR, AP1_RGB2Y);

        printf("%.11f %.11f %.11f\n", RRT_SAT_MAT[0][0], RRT_SAT_MAT[0][1], RRT_SAT_MAT[0][2]);
        printf("%.11f %.11f %.11f\n", RRT_SAT_MAT[1][0], RRT_SAT_MAT[1][1], RRT_SAT_MAT[1][2]);
        printf("%.11f %.11f %.11f\n", RRT_SAT_MAT[2][0], RRT_SAT_MAT[2][1], RRT_SAT_MAT[2][2]);

        //rgbPre = mul(RRT_SAT_MAT, rgbPre);
        //rgbPre = lerp(dot(rgbPre, AP1_RGB2Y).xxx, rgbPre, RRT_SAT_FACTOR.xxx);

        const float ODT_SAT_FACTOR = 0.93f;
        const glm::f32mat3x3 ODT_SAT_MAT = calc_sat_adjust_matrix(ODT_SAT_FACTOR, AP1_RGB2Y);

        printf("%.11f %.11f %.11f\n", ODT_SAT_MAT[0][0], ODT_SAT_MAT[0][1], ODT_SAT_MAT[0][2]);
        printf("%.11f %.11f %.11f\n", ODT_SAT_MAT[1][0], ODT_SAT_MAT[1][1], ODT_SAT_MAT[1][2]);
        printf("%.11f %.11f %.11f\n", ODT_SAT_MAT[2][0], ODT_SAT_MAT[2][1], ODT_SAT_MAT[2][2]);
    }

    TEST(ColorTestDouble, Run)
    {
        // "Glow" module constants
        const double RRT_GLOW_GAIN = 0.05;
        const double RRT_GLOW_MID = 0.08;

        // Red modifier constants
        const double RRT_RED_SCALE = 0.82;
        const double RRT_RED_PIVOT = 0.03;
        const double RRT_RED_HUE = 0.0;
        const double RRT_RED_WIDTH = 135.0;

        double AP1_RGB2Y[3] = { 0.2722287168,  0.6740817658,  0.0536895174 };

        // Desaturation contants
        const double RRT_SAT_FACTOR = 0.96;
        const glm::f64mat3x3 RRT_SAT_MAT = calc_sat_adjust_matrix(RRT_SAT_FACTOR, AP1_RGB2Y);

        printf("%.11lf %.11lf %.11lf\n", RRT_SAT_MAT[0][0], RRT_SAT_MAT[0][1], RRT_SAT_MAT[0][2]);
        printf("%.11lf %.11lf %.11lf\n", RRT_SAT_MAT[1][0], RRT_SAT_MAT[1][1], RRT_SAT_MAT[1][2]);
        printf("%.11lf %.11lf %.11lf\n", RRT_SAT_MAT[2][0], RRT_SAT_MAT[2][1], RRT_SAT_MAT[2][2]);

        //rgbPre = mul(RRT_SAT_MAT, rgbPre);
        //rgbPre = lerp(dot(rgbPre, AP1_RGB2Y).xxx, rgbPre, RRT_SAT_FACTOR.xxx);

        const double ODT_SAT_FACTOR = 0.93;
        const glm::f64mat3x3 ODT_SAT_MAT = calc_sat_adjust_matrix(ODT_SAT_FACTOR, AP1_RGB2Y);

        printf("%.11lf %.11lf %.11lf\n", ODT_SAT_MAT[0][0], ODT_SAT_MAT[0][1], ODT_SAT_MAT[0][2]);
        printf("%.11lf %.11lf %.11lf\n", ODT_SAT_MAT[1][0], ODT_SAT_MAT[1][1], ODT_SAT_MAT[1][2]);
        printf("%.11lf %.11lf %.11lf\n", ODT_SAT_MAT[2][0], ODT_SAT_MAT[2][1], ODT_SAT_MAT[2][2]);
    }
}