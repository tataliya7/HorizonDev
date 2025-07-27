#include "SceneView.h"

namespace Horizon
{
    Vector2f ComputeViewSpaceDepthToNDCSpaceDepthTransform(const Matrix4x4f& projectionMatrix)
    {
        // Perspective projection matrix (right-handed coordinate system):
        // | A  0  0  0 |
        // | 0  B  0  0 |
        // | 0  0  C  D |
        // | 0  0 -1  0 |
        //
        // | Xclip |   | A  0  0  0 | | Xview |
        // | Yclip | = | 0  B  0  0 | | Yview |
        // | Zclip |   | 0  0  C  D | | Zview |
        // | Wclip |   | 0  0 -1  0 | | Wview |
        //
        // Zclip = C * Zview + D * Wview
        // Wclip = -Zview
        // Wview = 1
        // Zndc = Zclip / Wclip = -(C + D / Zview)

        Vector2f transform = Vector2f(projectionMatrix[2][2], projectionMatrix[3][2]);
        return transform;
    }

    void CameraTransformations::Finalize()
    {
        viewToWorldMatrix = Math::InverseMatrix(worldToViewMatrix);
        clipToViewMatrix = Math::InverseMatrix(viewToClipMatrix);
        worldToClipMatrix = viewToClipMatrix * worldToViewMatrix;
        clipToWorldMatrix = viewToWorldMatrix * clipToViewMatrix;
        viewSpaceDepthToNDCSpaceDepthTransform = ComputeViewSpaceDepthToNDCSpaceDepthTransform(viewToClipMatrix);
    }

    SceneView::SceneView(const SceneViewDescription& description)
        : scene(description.scene)
        , renderSettings(description.renderSettings)
        , deltaTimeInSeconds(description.deltaTimeInSeconds)
        , frameIndex(description.frameIndex)
        , reset(description.reset)
        , enableInfiniteFarClippingPlane(false)
        , cameraPosition(description.cameraPosition)
        , cameraRotation(description.cameraRotation)
        , cameraUpVector(description.cameraUpVector)
        , cameraRightVector(description.cameraRightVector)
        , cameraForwardVector(description.cameraForwardVector)
        , verticalFOV(description.verticalFOV)
        , tanHalfVerticalFOV(std::tan(verticalFOV * 0.5f))
        , aspectRatio(description.aspectRatio)
        , nearClippingPlane(description.nearClippingPlane)
        , farClippingPlane(description.farClippingPlane)
        , backgroundColor(description.backgroundColor)
        , targetWidth(description.targetWidth)
        , targetHeight(description.targetHeight)
        , targetTexture(description.targetTexture)
        , displayWidth(description.displayWidth)
        , displayHeight(description.displayHeight)
        , displayTexture(description.displayTexture)
        , swapChain(description.swapChain)
        , cursorPosition(description.cursorPosition)
    {
        transformations.Update(cameraPosition, cameraRotation, verticalFOV, aspectRatio, nearClippingPlane, farClippingPlane);
        SetupViewFrustum();
    }
}