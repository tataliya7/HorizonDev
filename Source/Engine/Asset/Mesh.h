

namespace HE
{
    struct PreviewImage
    {
        /* All values of 2 are really NUM_ICON_SIZES */
        unsigned int w[2];
        unsigned int h[2];
        short flag[2];
        short changed_timestamp[2];
        unsigned int* rect[2];

        /* Runtime-only data. */
        struct GPUTexture* gputexture[2];
        /** Used by previews outside of ID context. */
        int icon_id;
    };

    struct Material
    {
        float roughness;
        float metallic;

        struct PreviewImage* previewImage;
    };

    struct Face
    {

    };

    struct Mesh
    {
        Material** materials;

        uint32 numFaces;
        uint32 numVertices;
    };
}