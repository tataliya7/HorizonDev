// #include "MeshSimplification.h"
//
// namespace Horizon::MeshSimplification
// {
//     static void RemoveTriangle(const MeshSimplificationContext& context, uint32 triangleIndex)
//     {
//         assert(triangleIndex <= context.triangleCount);
//         if (context.removedTriangles[triangleIndex])
//         {
//             context.removedTriangles[triangleIndex] = true;
//             context.triangleCount--;
//
//             for (uint32 i = 0; i < 3; i++)
//             {
//
//             }
//         }
//     }
//
//     void f()
//     {
//         for (uint32 i = 0; i < vertexCount; i++)
//         {
//
//         }
//
// 	    uint32 outputTriangleIndex = 0;
//         for (uint32 triangleIndex = 0; triangleIndex < triangleCount; triangleIndex++)
//         {
//             if (removedTriangles[triangleIndex])
//             {
//                 for (uint32 i = 0; i < 3; i++)
//                 {
//                     uint32 vertexIndex = indices[triangleIndex * 3 + i];
//                     indices[outputTriangleIndex * 3 + i] = VertRefCount[vertexIndex];
//                 }
//             }
// 			materialIndexes[outputTriangleIndex] = materialIndexes[triangleIndex];
//             outputTriangleIndex += 1;
//         }
//     }
//
//     bool SimplifyMesh()
//     {
//         assert(index_count % 3 == 0);
//
//
//     }
// }