#include "SceneView.h"

namespace Horizon
{
    Vector2 ComputeViewSpaceDepthToNDCSpaceDepthTransform(const Matrix4x4& projectionMatrix)
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

        Vector2 transform = Vector2(projectionMatrix[2][2], projectionMatrix[3][2]);
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
}