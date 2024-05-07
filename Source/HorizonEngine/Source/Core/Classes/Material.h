
{
    struct Material
    {
        std::string name;

        // BxDF
        Vector4 baseColor = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
        float metallic = 0.0f;
        float roughness = 0.5f;
        float specular = 0.5f;
        float specularTint = 0.0f;
        float transmission = 0.0f;
        float transmissionRoughness = 0.0f;
        float clearcoat = 0.0f;
        float clearcoatRoughness = 0.0f;
        Vector4 emission = Vector4(0.0f, 0.0f, 0.0f, 1.0f);
        float emissionStrength = 1.0f;
        float alpha = 1.0f;

        // SSS
        Vector4 sssSurfaceAlbedo = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
        Vector4 sssMFP = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
        float secondRoughness = 0.5f;
        float lobeMix = 0.0f;

        enum TextureSlot
        {
            BaseColorMap,
            MetallicRoughnessMap,
            SpecularGlossinessMap,
            NormalMap,
            EmissiveMap,
            Count
        };

        struct TextureMap
        {
            std::string path;
            bool used = false;
            RenderBackendTextureHandle gpuTexture;
        };
        TextureMap textures[16];
        bool useMetallicRoughnessWorkflow = false;
    };
}
