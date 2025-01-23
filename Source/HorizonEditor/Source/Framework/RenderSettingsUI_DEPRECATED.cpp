
namespace Horizon
{
    void HorizonEditor::DrawRenderSettingsWindow(bool* open)
    {
        auto& renderSettings = ((RenderSystem*)renderEngine)->GetRealTimeRendererSettings_Deprecated();

        if (ImGui::Begin("Render Settings", open))
        {
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
            ImGui::Columns(2);
            ImGui::Separator();

            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Renderer");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);

            static const char* rendererTypeNames[] = { "Horizon Renderer (Real-Time)", "Horizon Renderer (Path Tracing)" };
            int rendererType = (int)renderSettings.rendererType;
            ImGui::Combo("##Renderer", &rendererType, rendererTypeNames, IM_ARRAYSIZE(rendererTypeNames));
            renderSettings.rendererType = (RendererType)rendererType;

            ImGui::Columns(1);
            ImGui::Separator();
            ImGui::PopStyleVar();

            if (ImGui::CollapsingHeader("General", ImGuiTreeNodeFlags_None))
            {
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
                ImGui::Columns(2);
                ImGui::Separator();

                ImGui::AlignTextToFramePadding();
                ImGui::TextUnformatted("Fixed Pre-Exposure");
                ImGui::NextColumn();
                ImGui::PushItemWidth(-1);
                if (ImGui::Checkbox("##FixedPreExposure", &renderSettings.fixedPreExposureEnabled))
                {

                }
                ImGui::PopItemWidth();
                ImGui::NextColumn();

                ImGui::AlignTextToFramePadding();
                ImGui::TextUnformatted("Pre-Exposure");
                ImGui::NextColumn();
                ImGui::PushItemWidth(-1);
                if (ImGui::DragFloat("##PreExposure", &renderSettings.fixedPreExposure))
                {

                }
                ImGui::PopItemWidth();
                ImGui::NextColumn();

                ImGui::AlignTextToFramePadding();
                ImGui::TextUnformatted("Indirect Lighting Tint");
                ImGui::NextColumn();
                ImGui::PushItemWidth(-1);
                if (ImGui::ColorEdit3("##indirectLightingColor", &renderSettings.indirectLightingColor.x))
                {

                }
                ImGui::PopItemWidth();
                ImGui::NextColumn();

                ImGui::AlignTextToFramePadding();
                ImGui::TextUnformatted("Indirect Lighting Intensity");
                ImGui::NextColumn();
                ImGui::PushItemWidth(-1);
                if (ImGui::DragFloat("##indirectLightingIntensity", &renderSettings.indirectLightingIntensity, 0.01f, 0.0f, 1000.0f))
                {

                }
                ImGui::PopItemWidth();
                ImGui::NextColumn();

                ImGui::Columns(1);
                ImGui::Separator();
                ImGui::PopStyleVar();
            }

#if HE_ENBALE_STREAMLINE_SUPPORT
            if (ImGui::CollapsingHeader("NVIDIA Reflex", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
                ImGui::Columns(2);
                ImGui::Separator();

                ImGui::AlignTextToFramePadding();
                ImGui::TextUnformatted("Mode");
                ImGui::NextColumn();
                ImGui::PushItemWidth(-1);
                const char* items[] = { "Off", "On", "On+Boost" };
                int item = (int)renderPipelineSettings.reflexMode;
                ImGui::Combo("##ReflexMode", &item, items, IM_ARRAYSIZE(items));

                if (item == 0)
                {
                    sl::ReflexOptions reflexOptions = {};
                    reflexOptions.mode = sl::ReflexMode::eOff;
                    reflexOptions.frameLimitUs = 0;
                    reflexOptions.useMarkersToOptimize = true;
                    if (SL_FAILED(result, slReflexSetOptions(reflexOptions)))
                    {
                        LogError(GLogger, std::format("slReflexSetOptions, error code: {}", (int32)result));
                    }
                    else
                    {
                        renderPipelineSettings.reflexMode = (NVIDIAReflexMode)item;
                    }
                }
                else if (item == 1)
                {
                    sl::ReflexOptions reflexOptions = {};
                    reflexOptions.mode = sl::ReflexMode::eLowLatency;
                    reflexOptions.frameLimitUs = 0;
                    reflexOptions.useMarkersToOptimize = true;
                    if (SL_FAILED(result, slReflexSetOptions(reflexOptions)))
                    {
                        LogError(GLogger, std::format("slReflexSetOptions, error code: {}", (int32)result));
                    }
                    else
                    {
                        renderPipelineSettings.reflexMode = (NVIDIAReflexMode)item;
                    }
                }
                else if (item == 2)
                {
                    sl::ReflexOptions reflexOptions = {};
                    reflexOptions.mode = sl::ReflexMode::eLowLatencyWithBoost;
                    reflexOptions.frameLimitUs = 0;
                    reflexOptions.useMarkersToOptimize = true;
                    if (SL_FAILED(result, slReflexSetOptions(reflexOptions)))
                    {
                        LogError(GLogger, std::format("slReflexSetOptions, error code: {}", (int32)result));
                    }
                    else
                    {
                        renderPipelineSettings.reflexMode = (NVIDIAReflexMode)item;
                    }
                }
                else
                {
                    assert(0);
                }

                ImGui::PopItemWidth();
                ImGui::NextColumn();

                ImGui::Columns(1);
                ImGui::Separator();
                ImGui::PopStyleVar();
            }
#endif
            if (ImGui::CollapsingHeader("Shadows", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
                ImGui::Columns(2);
                ImGui::Separator();

                ImGui::AlignTextToFramePadding();
                ImGui::TextUnformatted("Technique");
                ImGui::NextColumn();
                ImGui::PushItemWidth(-1);

                const char* items[] = { "None", "Screen Space Shadows", "Ray Tracing Shadows" };
                static int item = 0;
                ImGui::Combo("##ShadowsTechnique", &item, items, IM_ARRAYSIZE(items));
                renderSettings.shadowsTechnique = (ShadowsTechnique)item;

                ImGui::PopItemWidth();
                ImGui::NextColumn();

                ImGui::Columns(1);
                ImGui::Separator();
                ImGui::PopStyleVar();
            }

            if (ImGui::CollapsingHeader("Reflections", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
                ImGui::Columns(2);
                ImGui::Separator();

                ImGui::AlignTextToFramePadding();
                ImGui::TextUnformatted("Technique");
                ImGui::NextColumn();
                ImGui::PushItemWidth(-1);

                const char* items[] = { "None", "Screen Space Reflections", "Ray Tracing Reflections" };
                static int item = 0;
                ImGui::Combo("##ReflectionsTechnique", &item, items, IM_ARRAYSIZE(items));
                renderSettings.reflectionsTechnique = (ReflectionsTechnique)item;

                ImGui::PopItemWidth();
                ImGui::NextColumn();

                ImGui::AlignTextToFramePadding();
                ImGui::TextUnformatted("Denosing");
                ImGui::NextColumn();
                ImGui::PushItemWidth(-1);
                if (ImGui::Checkbox("##SSRDenosing", &renderSettings.ssrSettings.denosingEnabled))
                {

                }
                ImGui::PopItemWidth();
                ImGui::NextColumn();

                ImGui::AlignTextToFramePadding();
                ImGui::TextUnformatted("Quality");
                ImGui::NextColumn();
                ImGui::PushItemWidth(-1);

                const char* items2[] = { "Low", "Medium", "High", "    Epic" };
                static int item2 = 0;
                ImGui::Combo("##SSRQuality", &item2, items2, IM_ARRAYSIZE(items2));
                renderSettings.ssrSettings.qualiy = (ScreenSpaceReflectionsQuality)item2;

                ImGui::PopItemWidth();
                ImGui::NextColumn();

                ImGui::Columns(1);
                ImGui::Separator();
                ImGui::PopStyleVar();
            }

            if (ImGui::CollapsingHeader("Ambient Occlusion", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
                ImGui::Columns(2);
                ImGui::Separator();

                bool gtaoEnabled = (renderSettings.ambientOcclusionTechnique == AmbientOcclusionTechnique::GroundTruthAmbientOcclusion) ? true : false;
                ImGui::AlignTextToFramePadding();
                ImGui::TextUnformatted("Enable");
                ImGui::NextColumn();
                ImGui::PushItemWidth(-1);
                if (ImGui::Checkbox("##Enable", &gtaoEnabled))
                {
                    if (gtaoEnabled)
                    {
                        renderSettings.ambientOcclusionTechnique = AmbientOcclusionTechnique::GroundTruthAmbientOcclusion;
                    }
                    else
                    {
                        renderSettings.ambientOcclusionTechnique = AmbientOcclusionTechnique::None;
                    }
                }
                ImGui::PopItemWidth();
                ImGui::NextColumn();

                ImGui::AlignTextToFramePadding();
                ImGui::TextUnformatted("Radius");
                ImGui::NextColumn();
                ImGui::PushItemWidth(-1);
                if (ImGui::DragFloat("##Radius", &renderSettings.gtaoSettings.radius))
                {

                }
                ImGui::PopItemWidth();
                ImGui::NextColumn();

                ImGui::AlignTextToFramePadding();
                ImGui::TextUnformatted("Factor");
                ImGui::NextColumn();
                ImGui::PushItemWidth(-1);
                if (ImGui::DragFloat("##Factor", &renderSettings.gtaoSettings.factor))
                {

                }
                ImGui::PopItemWidth();
                ImGui::NextColumn();

                ImGui::AlignTextToFramePadding();
                ImGui::TextUnformatted("Thickness");
                ImGui::NextColumn();
                ImGui::PushItemWidth(-1);
                if (ImGui::DragFloat("##Thickness", &renderSettings.gtaoSettings.thickness))
                {

                }
                ImGui::PopItemWidth();
                ImGui::NextColumn();

                ImGui::AlignTextToFramePadding();
                ImGui::TextUnformatted("Multiple-Bounce");
                ImGui::NextColumn();
                ImGui::PushItemWidth(-1);
                if (ImGui::Checkbox("##MultipleBounce", &renderSettings.gtaoSettings.multiBounce))
                {

                }
                ImGui::PopItemWidth();
                ImGui::NextColumn();

                ImGui::Columns(1);
                ImGui::Separator();
                ImGui::PopStyleVar();
            }

            if (ImGui::CollapsingHeader("Antialiasing", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
                ImGui::Columns(2);
                ImGui::Separator();

                ImGui::AlignTextToFramePadding();
                ImGui::TextUnformatted("Technique");
                ImGui::NextColumn();
                ImGui::PushItemWidth(-1);

                const char* items[] = { "None", "TAA", "NVIDIA DLAA" };
                static int item = 0;
                ImGui::Combo("##AntialiasingTechnique", &item, items, IM_ARRAYSIZE(items));
                renderSettings.antialiasingTechnique = (AntialiasingTechnique)item;

                ImGui::PopItemWidth();
                ImGui::NextColumn();

                ImGui::Columns(1);
                ImGui::Separator();
                ImGui::PopStyleVar();
            }

            if (ImGui::CollapsingHeader("Super Resolution", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
                ImGui::Columns(2);
                ImGui::Separator();

                ImGui::AlignTextToFramePadding();
                ImGui::TextUnformatted("Technique");
                ImGui::NextColumn();
                ImGui::PushItemWidth(-1);

                const char* items[] = { "None", "AMD FSR2", "NVIDIA DLSS" };
                int item = (int)renderSettings.superResolutionTechnique;
                ImGui::Combo("##SuperResolutionTechnique", &item, items, IM_ARRAYSIZE(items));
                renderSettings.superResolutionTechnique = (SuperResolutionTechnique)item;

                ImGui::PopItemWidth();
                ImGui::NextColumn();

                switch (item)
                {
                case (uint32)SuperResolutionTechnique::FSR2:
                {
                    ImGui::AlignTextToFramePadding();
                    ImGui::TextUnformatted("Quality Mode");
                    ImGui::NextColumn();
                    ImGui::PushItemWidth(-1);

                    static const char* fsr2QualityModeNames[] = { "Custom", "Quality", "Balanced", "Performance", "Ultra Performance" };
                    int fsr2QualityModeNameIndex = (int)renderSettings.fsr2Settings.qualityMode;
                    ImGui::Combo("##FSR2QualityMode", &fsr2QualityModeNameIndex, fsr2QualityModeNames, IM_ARRAYSIZE(fsr2QualityModeNames));
                    renderSettings.fsr2Settings.qualityMode = (FSR2QualityMode)fsr2QualityModeNameIndex;
                    ImGui::PopItemWidth();
                    ImGui::NextColumn();

                    if (renderSettings.fsr2Settings.qualityMode == FSR2QualityMode::Custom)
                    {
                        ImGui::AlignTextToFramePadding();
                        ImGui::TextUnformatted("Custom Upscale Ratio");
                        ImGui::NextColumn();
                        ImGui::PushItemWidth(-1);
                        if (ImGui::DragFloat("##FSR2UpscaleRatio", &renderSettings.fsr2Settings.customUpscaleRatio, 0.001f, 1.0f, 3.0f))
                        {

                        }
                        ImGui::PopItemWidth();
                        ImGui::NextColumn();
                    }

                    ImGui::AlignTextToFramePadding();
                    ImGui::TextUnformatted("Enable Sharpening");
                    ImGui::NextColumn();
                    ImGui::PushItemWidth(-1);
                    if (ImGui::Checkbox("##FSR2EnableSharpening", &renderSettings.fsr2Settings.useRCAS))
                    {

                    }
                    ImGui::PopItemWidth();
                    ImGui::NextColumn();

                    if (!renderSettings.fsr2Settings.useRCAS)
                    {
                        ImGui::BeginDisabled();
                    }
                    ImGui::AlignTextToFramePadding();
                    ImGui::TextUnformatted("Sharpeness");
                    ImGui::NextColumn();
                    ImGui::PushItemWidth(-1);
                    if (ImGui::DragFloat("##FSR2Sharpeness", &renderSettings.fsr2Settings.sharpeness, 0.001f, 0.0f, 1.0f))
                    {

                    }
                    ImGui::PopItemWidth();
                    ImGui::NextColumn();
                    if (!renderSettings.fsr2Settings.useRCAS)
                    {
                        ImGui::EndDisabled();
                    }
                } break;
                case (uint32)SuperResolutionTechnique::DLSSSuperResolution:
                {
                    ImGui::AlignTextToFramePadding();
                    ImGui::TextUnformatted("Quality Mode");
                    ImGui::NextColumn();
                    ImGui::PushItemWidth(-1);

                    static const char* dlssQualityModeNames[] = { "Off", "Auto", "Quality", "Balanced", "Performance", "Ultra Performance" };
                    static int dlssQualityModeNameIndex = 1;
                    ImGui::Combo("##DLSSQualityMode", &dlssQualityModeNameIndex, dlssQualityModeNames, IM_ARRAYSIZE(dlssQualityModeNames));
                    renderSettings.dlssSettings.qualityMode = (DLSSQualityMode)dlssQualityModeNameIndex;
                    ImGui::PopItemWidth();
                    ImGui::NextColumn();

                } break;
                default: break;
                }

                ImGui::Columns(1);
                ImGui::Separator();
                ImGui::PopStyleVar();
            }

            if (ImGui::CollapsingHeader((const char*)(u8"Post Proccesing"), ImGuiTreeNodeFlags_DefaultOpen))
            {
                // TODO: Visualize tone mapping curve
                if (ImGui::TreeNode("Tone Mapping"))
                {
                    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
                    ImGui::Columns(2);
                    ImGui::Separator();

                    ImGui::AlignTextToFramePadding();
                    ImGui::TextUnformatted("Tone Mapping Operator");
                    ImGui::NextColumn();
                    ImGui::PushItemWidth(-1);

                    static const char* toneMappingOperatorNames[] = { "Linear", "ACES" };
                    int toneMappingOperator = (int)renderSettings.toneMappingOperator;
                    ImGui::Combo("##ToneMappingOperator", &toneMappingOperator, toneMappingOperatorNames, IM_ARRAYSIZE(toneMappingOperatorNames));
                    renderSettings.toneMappingOperator = (ToneMappingOperatorType)toneMappingOperator;

                    ImGui::PopItemWidth();
                    ImGui::NextColumn();


                    ImGui::Columns(1);
                    ImGui::Separator();
                    ImGui::PopStyleVar();

                    ImGui::TreePop();
                }

                if (ImGui::TreeNode("Bloom"))
                {
                    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
                    ImGui::Columns(2);
                    ImGui::Separator();

                    ImGui::AlignTextToFramePadding();
                    ImGui::TextUnformatted("Intensity");
                    ImGui::NextColumn();
                    ImGui::PushItemWidth(-1);
                    if (ImGui::DragFloat("##BloomIntensity", &renderSettings.postProcessingSettings.bloomIntensity, 0.01f, 0.0f, 10.0f))
                    {

                    }
                    ImGui::PopItemWidth();
                    ImGui::NextColumn();

                    ImGui::AlignTextToFramePadding();
                    ImGui::TextUnformatted("Radius");
                    ImGui::NextColumn();
                    ImGui::PushItemWidth(-1);
                    if (ImGui::DragFloat("##BloomRadius", &renderSettings.postProcessingSettings.bloomRadius, 0.001f, 0.0f, 1.0f))
                    {

                    }
                    ImGui::PopItemWidth();
                    ImGui::NextColumn();

                    ImGui::Columns(1);
                    ImGui::Separator();
                    ImGui::PopStyleVar();

                    ImGui::TreePop();
                }

                if (ImGui::TreeNode("Lens Dirt"))
                {
                    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
                    ImGui::Columns(2);
                    ImGui::Separator();

                    ImGui::AlignTextToFramePadding();
                    ImGui::TextUnformatted("Intensity");
                    ImGui::NextColumn();
                    ImGui::PushItemWidth(-1);
                    if (ImGui::DragFloat("##LensDirtIntensity", &renderSettings.postProcessingSettings.lensDirtIntensity, 0.001f, 0.0f, 1.0f))
                    {

                    }
                    ImGui::PopItemWidth();
                    ImGui::NextColumn();

                    ImGui::AlignTextToFramePadding();
                    ImGui::TextUnformatted("Tint");
                    ImGui::NextColumn();
                    ImGui::PushItemWidth(-1);
                    if (ImGui::ColorEdit4("##LensDirtTint", &renderSettings.postProcessingSettings.lensDirtTint.x))
                    {

                    }
                    ImGui::PopItemWidth();
                    ImGui::NextColumn();

                    ImGui::Columns(1);
                    ImGui::Separator();
                    ImGui::PopStyleVar();

                    ImGui::TreePop();
                }

                if (ImGui::TreeNode("Lens Flare"))
                {
                    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
                    ImGui::Columns(2);
                    ImGui::Separator();

                    ImGui::AlignTextToFramePadding();
                    ImGui::TextUnformatted("Intensity");
                    ImGui::NextColumn();
                    ImGui::PushItemWidth(-1);
                    if (ImGui::DragFloat("##lensFlareIntensity", &renderSettings.postProcessingSettings.lensFlareIntensity, 0.001f, 0.0f, 1.0f))
                    {

                    }
                    ImGui::PopItemWidth();
                    ImGui::NextColumn();

                    ImGui::Columns(1);
                    ImGui::Separator();
                    ImGui::PopStyleVar();

                    ImGui::TreePop();
                }

                if (ImGui::TreeNode("Exposure"))
                {
                    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
                    ImGui::Columns(2);
                    ImGui::Separator();

                    const char* items[] = { "Fixed Exposure", "Auto Exposure" };
                    int item = (int)renderSettings.exposureMethod;

                    ImGui::AlignTextToFramePadding();
                    ImGui::TextUnformatted("Exposure Method");
                    ImGui::NextColumn();
                    ImGui::PushItemWidth(-1);
                    if (ImGui::Combo("##ExposureMethod", &item, items, IM_ARRAYSIZE(items)))
                    {
                        renderSettings.exposureMethod = (ExposureMethod)item;
                    }
                    ImGui::PopItemWidth();
                    ImGui::NextColumn();

                    ImGui::AlignTextToFramePadding();
                    ImGui::TextUnformatted("Fixed Exposure Value");
                    ImGui::NextColumn();
                    ImGui::PushItemWidth(-1);
                    if (ImGui::DragFloat("##fixedExposureValue", &renderSettings.postProcessingSettings.fixedExposureValue))
                    {

                    }
                    ImGui::PopItemWidth();
                    ImGui::NextColumn();

                    ImGui::AlignTextToFramePadding();
                    ImGui::TextUnformatted("Exposure Compensation");
                    ImGui::NextColumn();
                    ImGui::PushItemWidth(-1);
                    if (ImGui::DragFloat("##autoExposureExposureCompensation", &renderSettings.postProcessingSettings.autoExposureExposureCompensation))
                    {

                    }
                    ImGui::PopItemWidth();
                    ImGui::NextColumn();

                    ImGui::AlignTextToFramePadding();
                    ImGui::TextUnformatted("Min Exposure Value");
                    ImGui::NextColumn();
                    ImGui::PushItemWidth(-1);
                    if (ImGui::DragFloat("##autoExposureMinExposureValue", &renderSettings.postProcessingSettings.autoExposureMinExposureValue))
                    {

                    }
                    ImGui::PopItemWidth();
                    ImGui::NextColumn();

                    ImGui::AlignTextToFramePadding();
                    ImGui::TextUnformatted("Max Exposure Value");
                    ImGui::NextColumn();
                    ImGui::PushItemWidth(-1);
                    if (ImGui::DragFloat("##autoExposureMaxExposureValue", &renderSettings.postProcessingSettings.autoExposureMaxExposureValue))
                    {

                    }
                    ImGui::PopItemWidth();
                    ImGui::NextColumn();

                    ImGui::AlignTextToFramePadding();
                    ImGui::TextUnformatted("Speed Dark to Bright");
                    ImGui::NextColumn();
                    ImGui::PushItemWidth(-1);
                    if (ImGui::DragFloat("##autoExposureSpeedDarkToBright", &renderSettings.postProcessingSettings.autoExposureSpeedDarkToBright))
                    {

                    }
                    ImGui::PopItemWidth();
                    ImGui::NextColumn();


                    ImGui::AlignTextToFramePadding();
                    ImGui::TextUnformatted("Speed Bright to Dark");
                    ImGui::NextColumn();
                    ImGui::PushItemWidth(-1);
                    if (ImGui::DragFloat("##autoExposureSpeedBrightToDark", &renderSettings.postProcessingSettings.autoExposureSpeedBrightToDark))
                    {

                    }
                    ImGui::PopItemWidth();
                    ImGui::NextColumn();

                    ImGui::AlignTextToFramePadding();
                    ImGui::TextUnformatted("Low Percent");
                    ImGui::NextColumn();
                    ImGui::PushItemWidth(-1);
                    if (ImGui::DragFloat("##autoExposureHistogramLowerPercentage", &renderSettings.postProcessingSettings.autoExposureHistogramLowerPercentage, 0.0001f, 0.0f, 1.0f))
                    {

                    }
                    ImGui::PopItemWidth();
                    ImGui::NextColumn();

                    ImGui::AlignTextToFramePadding();
                    ImGui::TextUnformatted("High Percent");
                    ImGui::NextColumn();
                    ImGui::PushItemWidth(-1);
                    if (ImGui::DragFloat("##autoExposureHistogramHigherPercentage", &renderSettings.postProcessingSettings.autoExposureHistogramHigherPercentage, 0.0001f, 0.0f, 1.0f))
                    {

                    }
                    ImGui::PopItemWidth();
                    ImGui::NextColumn();

                    ImGui::AlignTextToFramePadding();
                    ImGui::TextUnformatted("Histogram Min EV100");
                    ImGui::NextColumn();
                    ImGui::PushItemWidth(-1);
                    if (ImGui::DragFloat("##autoExposureHistogramMinEV100", &renderSettings.postProcessingSettings.autoExposureHistogramMinEV100))
                    {

                    }
                    ImGui::PopItemWidth();
                    ImGui::NextColumn();

                    ImGui::AlignTextToFramePadding();
                    ImGui::TextUnformatted("Histogram Max EV100");
                    ImGui::NextColumn();
                    ImGui::PushItemWidth(-1);
                    if (ImGui::DragFloat("##autoExposureHistogramMaxEV100", &renderSettings.postProcessingSettings.autoExposureHistogramMaxEV100))
                    {

                    }
                    ImGui::PopItemWidth();
                    ImGui::NextColumn();

                    ImGui::Columns(1);
                    ImGui::Separator();
                    ImGui::PopStyleVar();

                    ImGui::TreePop();
                }

                if (ImGui::TreeNode("Local Exposure"))
                {
                    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
                    ImGui::Columns(2);
                    ImGui::Separator();

                    ImGui::AlignTextToFramePadding();
                    ImGui::TextUnformatted("Enable");
                    ImGui::NextColumn();
                    ImGui::PushItemWidth(-1);
                    if (ImGui::Checkbox("##LocalToneMapping", &renderSettings.postProcessingSettings.localToneMappingEnabled))
                    {

                    }
                    ImGui::PopItemWidth();
                    ImGui::NextColumn();

                    ImGui::AlignTextToFramePadding();
                    ImGui::TextUnformatted("Shadows");
                    ImGui::NextColumn();
                    ImGui::PushItemWidth(-1);
                    if (ImGui::DragFloat("##LocalToneMappingShadows", &renderSettings.postProcessingSettings.localToneMappingShadows, 0.01f, 0.0f, 4.0f))
                    {

                    }
                    ImGui::PopItemWidth();
                    ImGui::NextColumn();

                    ImGui::AlignTextToFramePadding();
                    ImGui::TextUnformatted("Highlights");
                    ImGui::NextColumn();
                    ImGui::PushItemWidth(-1);
                    if (ImGui::DragFloat("##LocalToneMappingHighlights", &renderSettings.postProcessingSettings.localToneMappingHighlights, 0.01f, 0.0f, 4.0f))
                    {

                    }
                    ImGui::PopItemWidth();
                    ImGui::NextColumn();

                    ImGui::AlignTextToFramePadding();
                    ImGui::TextUnformatted("Coarsest Mip Level");
                    ImGui::NextColumn();
                    ImGui::PushItemWidth(-1);
                    if (ImGui::DragInt("##LocalToneMappingCoarsestMipLevel", &renderSettings.postProcessingSettings.localToneMappingCoarsestMipLevel, 1.0f, 0, 32))
                    {

                    }
                    ImGui::PopItemWidth();
                    ImGui::NextColumn();

                    ImGui::AlignTextToFramePadding();
                    ImGui::TextUnformatted("Display Mip Level");
                    ImGui::NextColumn();
                    ImGui::PushItemWidth(-1);
                    if (ImGui::DragInt("##LocalToneMappingDisplayMipLevel", &renderSettings.postProcessingSettings.localToneMappingDisplayMipLevel, 1.0f, 0, 32))
                    {

                    }
                    ImGui::PopItemWidth();
                    ImGui::NextColumn();

                    ImGui::AlignTextToFramePadding();
                    ImGui::TextUnformatted("Exposure Preference Sigma");
                    ImGui::NextColumn();
                    ImGui::PushItemWidth(-1);
                    if (ImGui::DragFloat("##LocalToneMappingPreferenceSigma", &renderSettings.postProcessingSettings.localToneMappingPreferenceSigma))
                    {

                    }
                    ImGui::PopItemWidth();
                    ImGui::NextColumn();

                    ImGui::Columns(1);
                    ImGui::Separator();
                    ImGui::PopStyleVar();

                    ImGui::TreePop();
                }

                if (ImGui::TreeNode("Depth Of Field"))
                {
                    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
                    ImGui::Columns(2);
                    ImGui::Separator();

                    ImGui::AlignTextToFramePadding();
                    ImGui::TextUnformatted("Scale");
                    ImGui::NextColumn();
                    ImGui::PushItemWidth(-1);
                    if (ImGui::DragFloat("##DOFScale", &renderSettings.postProcessingSettings.dofScale))
                    {

                    }
                    ImGui::PopItemWidth();
                    ImGui::NextColumn();

                    ImGui::AlignTextToFramePadding();
                    ImGui::TextUnformatted("Focal Distance");
                    ImGui::NextColumn();
                    ImGui::PushItemWidth(-1);
                    if (ImGui::DragFloat("##DOFFocalDistance", &renderSettings.postProcessingSettings.dofFocalDistance))
                    {

                    }
                    ImGui::PopItemWidth();
                    ImGui::NextColumn();

                    ImGui::AlignTextToFramePadding();
                    ImGui::TextUnformatted("Focal Region");
                    ImGui::NextColumn();
                    ImGui::PushItemWidth(-1);
                    if (ImGui::DragFloat("##DOFFocalRegion", &renderSettings.postProcessingSettings.dofFocalRegion))
                    {

                    }
                    ImGui::PopItemWidth();
                    ImGui::NextColumn();

                    ImGui::AlignTextToFramePadding();
                    ImGui::TextUnformatted("Focal Near Transition Region");
                    ImGui::NextColumn();
                    ImGui::PushItemWidth(-1);
                    if (ImGui::DragFloat("##DOFFocalNearTransitionRegion", &renderSettings.postProcessingSettings.dofNearTransitionRegion))
                    {

                    }
                    ImGui::PopItemWidth();
                    ImGui::NextColumn();

                    ImGui::AlignTextToFramePadding();
                    ImGui::TextUnformatted("Focal Far Transition Region");
                    ImGui::NextColumn();
                    ImGui::PushItemWidth(-1);
                    if (ImGui::DragFloat("##DOFFocalFarTransitionRegion", &renderSettings.postProcessingSettings.dofFarTransitionRegion))
                    {

                    }
                    ImGui::PopItemWidth();
                    ImGui::NextColumn();

                    ImGui::AlignTextToFramePadding();
                    ImGui::TextUnformatted("Focal Near Region Blur Size");
                    ImGui::NextColumn();
                    ImGui::PushItemWidth(-1);
                    if (ImGui::DragFloat("##DOFFocalNearRegionBlurSize", &renderSettings.postProcessingSettings.dofNearRegionBlurSize))
                    {

                    }
                    ImGui::PopItemWidth();
                    ImGui::NextColumn();

                    ImGui::AlignTextToFramePadding();
                    ImGui::TextUnformatted("Focal Far Region Blur Size");
                    ImGui::NextColumn();
                    ImGui::PushItemWidth(-1);
                    if (ImGui::DragFloat("##DOFFocalFarRegionBlurSize", &renderSettings.postProcessingSettings.dofFarRegionBlurSize))
                    {

                    }
                    ImGui::PopItemWidth();
                    ImGui::NextColumn();

                    ImGui::Columns(1);
                    ImGui::Separator();
                    ImGui::PopStyleVar();

                    ImGui::TreePop();
                }

                if (ImGui::TreeNode("Chromatic Aberration"))
                {
                    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
                    ImGui::Columns(2);
                    ImGui::Separator();

                    ImGui::AlignTextToFramePadding();
                    ImGui::TextUnformatted("Intensity");
                    ImGui::NextColumn();
                    ImGui::PushItemWidth(-1);
                    if (ImGui::DragFloat("##ChromaticAberrationIntensity", &renderSettings.postProcessingSettings.chromaticAberrationIntensity))
                    {

                    }
                    ImGui::PopItemWidth();
                    ImGui::NextColumn();

                    ImGui::AlignTextToFramePadding();
                    ImGui::TextUnformatted("Offset");
                    ImGui::NextColumn();
                    ImGui::PushItemWidth(-1);
                    if (ImGui::DragFloat("##ChromaticAberrationOffset", &renderSettings.postProcessingSettings.chromaticAberrationOffset))
                    {

                    }
                    ImGui::PopItemWidth();
                    ImGui::NextColumn();

                    ImGui::Columns(1);
                    ImGui::Separator();
                    ImGui::PopStyleVar();

                    ImGui::TreePop();
                }

                if (ImGui::TreeNode("Color Correction"))
                {
                    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
                    ImGui::Columns(2);
                    ImGui::Separator();

                    ImGui::AlignTextToFramePadding();
                    ImGui::TextUnformatted("Saturation");
                    ImGui::NextColumn();
                    ImGui::PushItemWidth(-1);
                    if (ImGui::ColorEdit4("##colorCorrectionSaturation", &renderSettings.postProcessingSettings.colorCorrectionSaturation.x))
                    {

                    }
                    ImGui::PopItemWidth();
                    ImGui::NextColumn();

                    ImGui::AlignTextToFramePadding();
                    ImGui::TextUnformatted("Contrast");
                    ImGui::NextColumn();
                    ImGui::PushItemWidth(-1);
                    if (ImGui::ColorEdit4("##colorCorrectionContrast", &renderSettings.postProcessingSettings.colorCorrectionContrast.x))
                    {

                    }
                    ImGui::PopItemWidth();
                    ImGui::NextColumn();

                    ImGui::AlignTextToFramePadding();
                    ImGui::TextUnformatted("Gamma");
                    ImGui::NextColumn();
                    ImGui::PushItemWidth(-1);
                    if (ImGui::ColorEdit4("##colorCorrectionGamma", &renderSettings.postProcessingSettings.colorCorrectionGamma.x))
                    {

                    }
                    ImGui::PopItemWidth();
                    ImGui::NextColumn();

                    ImGui::AlignTextToFramePadding();
                    ImGui::TextUnformatted("Gain");
                    ImGui::NextColumn();
                    ImGui::PushItemWidth(-1);
                    if (ImGui::ColorEdit4("##colorCorrectionGain", &renderSettings.postProcessingSettings.colorCorrectionGain.x))
                    {

                    }
                    ImGui::PopItemWidth();
                    ImGui::NextColumn();

                    ImGui::AlignTextToFramePadding();
                    ImGui::TextUnformatted("Offset");
                    ImGui::NextColumn();
                    ImGui::PushItemWidth(-1);
                    if (ImGui::ColorEdit4("##colorCorrectionOffset", &renderSettings.postProcessingSettings.colorCorrectionOffset.x))
                    {

                    }
                    ImGui::PopItemWidth();
                    ImGui::NextColumn();

                    ImGui::Columns(1);
                    ImGui::Separator();
                    ImGui::PopStyleVar();

                    ImGui::TreePop();
                }

                if (ImGui::TreeNode("Color Grading"))
                {
                    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
                    ImGui::Columns(2);
                    ImGui::Separator();

                    ImGui::AlignTextToFramePadding();
                    ImGui::TextUnformatted("White Balance");
                    ImGui::NextColumn();
                    ImGui::PushItemWidth(-1);
                    if (ImGui::DragFloat("##ColorGraingWhiteBalance", &renderSettings.postProcessingSettings.colorGradingWhiteBalanceColorTemperature, 0.1f, 1000.0f, 25000.0f))
                    {

                    }
                    ImGui::PopItemWidth();
                    ImGui::NextColumn();

                    ImGui::Columns(1);
                    ImGui::Separator();
                    ImGui::PopStyleVar();

                    ImGui::TreePop();
                }
            }
        }
        ImGui::End();
    }
}