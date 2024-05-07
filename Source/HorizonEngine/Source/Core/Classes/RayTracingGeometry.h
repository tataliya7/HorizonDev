
{
    enum class RayTracingGeometryState
    {
        Invalid = 0,
        BuildRequired = 1,
        UpdateRequired = 2,
        UpToDate = 3,
    };

    struct RayTracingGeometry
    {
        RayTracingGeometryState state;
        RenderBackendRayTracingAccelerationStructureHandle blas;

        bool IsUpToDate() const
        {
            return state == RayTracingGeometryState::UpToDate;
        }
    };
}
