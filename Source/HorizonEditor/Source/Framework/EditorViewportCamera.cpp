//#include "EditorCamera.h"
//
//namespace Horizon
//{
//    void EditorCamera::Update()
//    {
//        Quaternion orientation = Quaternion(Math::DegreesToRadians(euler));
//        viewMatrix = Math::Inverse(Math::Compose(position, orientation, Vector3(1.0f, 1.0f, 1.0f)));
//        projectionMatrix = glm::perspectiveLH(fieldOfView, aspectRatio, zNear, zFar);
//    }
//}