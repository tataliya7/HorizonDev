#include "HorizonEditor.h"
#include "HorizonEditorUI.h"

#include <imgui.h>
#include <imgui_internal.h>
#include <backends/imgui_impl_glfw.h>

#include "Gizmo.h"

#include "imnodes.h"
#include "imnodes_internal.h"

#include <optick.h>

#include "RenderDocPlugin.h"

#include "InspectorUI_DEPRECATED.h"

namespace Horizon
{
    static void SetColorTheme_Dark(ImGuiStyle& style)
    {

    }

    static void SetColorTheme_Light(ImGuiStyle& style)
    {
        // light style from Pacôme Danhiez (user itamago) https://github.com/ocornut/imgui/pull/511#issuecomment-175719267
        style.WindowRounding = 2.0f;
        style.ScrollbarRounding = 3.0f;
        style.GrabRounding = 2.0f;
        style.AntiAliasedLines = true;
        style.AntiAliasedFill = true;
        style.WindowRounding = 2;
        style.ChildRounding = 2;
        style.ScrollbarSize = 16;
        style.ScrollbarRounding = 3;
        style.GrabRounding = 2;
        style.ItemSpacing.x = 10;
        style.ItemSpacing.y = 4;
        style.IndentSpacing = 22;
        style.FramePadding.x = 6;
        style.FramePadding.y = 4;
        style.Alpha = 1.0f;
        style.FrameRounding = 3.0f;
        style.TabBorderSize = 0.0f;

        style.Colors[ImGuiCol_Text] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
        style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
        style.Colors[ImGuiCol_WindowBg] = ImVec4(0.94f, 0.94f, 0.94f, 0.94f);
        style.Colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        style.Colors[ImGuiCol_PopupBg] = ImVec4(1.00f, 1.00f, 1.00f, 0.94f);
        style.Colors[ImGuiCol_Border] = ImVec4(0.00f, 0.00f, 0.00f, 0.39f);
        style.Colors[ImGuiCol_BorderShadow] = ImVec4(1.00f, 1.00f, 1.00f, 0.10f);
        style.Colors[ImGuiCol_FrameBg] = ImVec4(1.00f, 1.00f, 1.00f, 0.94f);
        style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
        style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
        style.Colors[ImGuiCol_TitleBg] = ImVec4(0.96f, 0.96f, 0.96f, 1.00f);
        style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(1.00f, 1.00f, 1.00f, 0.51f);
        style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.82f, 0.82f, 0.82f, 1.00f);
        style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.86f, 0.86f, 0.86f, 1.00f);
        style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.98f, 0.98f, 0.98f, 0.53f);
        style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.69f, 0.69f, 0.69f, 1.00f);
        style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.59f, 0.59f, 0.59f, 1.00f);
        style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.49f, 0.49f, 0.49f, 1.00f);
        //style.Colors[ImGuiCol_ComboBg] = ImVec4(0.86f, 0.86f, 0.86f, 0.99f);
        style.Colors[ImGuiCol_CheckMark] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
        style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.24f, 0.52f, 0.88f, 1.00f);
        style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
        style.Colors[ImGuiCol_Button] = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
        style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
        style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.06f, 0.53f, 0.98f, 1.00f);
        style.Colors[ImGuiCol_Header] = ImVec4(0.26f, 0.59f, 0.98f, 0.31f);
        style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
        style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
        style.Colors[ImGuiCol_Separator] = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
        style.Colors[ImGuiCol_SeparatorHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.78f);
        style.Colors[ImGuiCol_SeparatorActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
        style.Colors[ImGuiCol_ResizeGrip] = ImVec4(1.00f, 1.00f, 1.00f, 0.50f);
        style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
        style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
        style.Colors[ImGuiCol_Tab] = ImVec4(0.59f, 0.59f, 0.59f, 1.0f);
        style.Colors[ImGuiCol_TabHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
        style.Colors[ImGuiCol_TabActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
        style.Colors[ImGuiCol_TabUnfocused] = ImVec4(0.59f, 0.59f, 0.59f, 1.0f);
        style.Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
        style.Colors[ImGuiCol_PlotLines] = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
        style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
        style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
        style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
        style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
        style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.20f, 0.20f, 0.20f, 0.35f);
    }

    void HorizonEditor::SetColorTheme(HorizonEditorColorTheme theme) const
    {
        ImGui::SetCurrentContext(imguiContext);

        ImGui::StyleColorsDark();
        ImGuiStyle& style = ImGui::GetStyle();

        switch (theme)
        {
        case HorizonEditorColorTheme::Dark:
            SetColorTheme_Dark(style);
            break;
        case HorizonEditorColorTheme::Light:
            SetColorTheme_Light(style);
            break;
        default:
            std::unreachable();
            break;
        }
    }

    void HorizonEditor::InitializeImGuiContext()
    {
        IMGUI_CHECKVERSION();

        // Create ImGuiContext
        imguiContext = ImGui::CreateContext();
        ImGui::SetCurrentContext(imguiContext);

        ImGuiIO& io = ImGui::GetIO();
        IM_ASSERT(io.BackendRendererUserData == NULL && "Already initialized a renderer backend!");
        io.BackendRendererName = "Horizon Engine";
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking

        // TODO: rewrite this
        io.IniFilename = "../../../Assets/Test/imgui.ini";
        io.IniSavingRate = 600;
        io.FontDefault = io.Fonts->AddFontFromFileTTF("../../../Assets/Fonts/OpenSans/OpenSans-Regular.ttf", 26.0f);

        // Upload fonts
        {
            if (true)
            {
                io.FontDefault = io.Fonts->AddFontFromFileTTF("../../../Assets/Fonts/OpenSans/OpenSans-Regular.ttf", 26.0f);
            }
            else
            {

            }

            unsigned char* fontTextureData = nullptr;
            int fontTextureWidth = 0;
            int fontTextureHeight = 0;
            RenderBackendTextureFormat fontTextureFormat = RenderBackendTextureFormat::R8G8B8A8Unorm;

            io.Fonts->GetTexDataAsRGBA32(&fontTextureData, &fontTextureWidth, &fontTextureHeight);

            RenderBackendTextureDesc fontTextureDesc = RenderBackendTextureDesc::Create2D(
                fontTextureWidth,
                fontTextureHeight,
                fontTextureFormat,
                RenderBackendTextureCreateFlags::ShaderResource);

            fontTexture = renderBackend->CreateTexture(&fontTextureDesc, fontTextureData, "FontTexture");

            io.Fonts->SetTexID(fontTexture.ToUnit64());
        }

        // Set color theme
        SetColorTheme(colorTheme);

        assert(window);
        ImGui_ImplGlfw_InitForOther(window->GetGLFWwindow(), true);

        gizmoOperationType = ImGuizmo::OPERATION::TRANSLATE;
    }

    //struct ConsoleLog
    //{
    //    std::string message;
    //};

    //struct Console
    //{
    //    char                  inputBuffer[256];
    //    ImVector<char*>       Items;
    //    ImVector<const char*> Commands;
    //    ImVector<char*>       History;
    //    int                   HistoryPos;    // -1: new line, 0..History.Size-1 browsing history.
    //    ImGuiTextFilter       Filter;
    //    bool                  AutoScroll;
    //    bool                  ScrollToBottom;

    //    // Portable helpers
    //    static int   Stricmp(const char* s1, const char* s2) { int d; while ((d = toupper(*s2) - toupper(*s1)) == 0 && *s1) { s1++; s2++; } return d; }
    //    static int   Strnicmp(const char* s1, const char* s2, int n) { int d = 0; while (n > 0 && (d = toupper(*s2) - toupper(*s1)) == 0 && *s1) { s1++; s2++; n--; } return d; }
    //    static char* Strdup(const char* s) { IM_ASSERT(s); size_t len = strlen(s) + 1; void* buf = malloc(len); IM_ASSERT(buf); return (char*)memcpy(buf, (const void*)s, len); }
    //    static void  Strtrim(char* s) { char* str_end = s + strlen(s); while (str_end > s && str_end[-1] == ' ') str_end--; *str_end = 0; }

    //    void    ClearLog()
    //    {
    //        for (int i = 0; i < Items.Size; i++)
    //            free(Items[i]);
    //        Items.clear();
    //    }

    //    void AddLog(const char* fmt, ...) IM_FMTARGS(2)
    //    {
    //        // FIXME-OPT
    //        char buf[1024];
    //        va_list args;
    //        va_start(args, fmt);
    //        vsnprintf(buf, IM_ARRAYSIZE(buf), fmt, args);
    //        buf[IM_ARRAYSIZE(buf) - 1] = 0;
    //        va_end(args);
    //        Items.push_back(Strdup(buf));
    //    }

    //    int TextEditCallback(ImGuiInputTextCallbackData* data)
    //    {
    //        //AddLog("cursor: %d, selection: %d-%d", data->CursorPos, data->SelectionStart, data->SelectionEnd);
    //        switch (data->EventFlag)
    //        {
    //        case ImGuiInputTextFlags_CallbackCompletion:
    //        {
    //            // Example of TEXT COMPLETION

    //            // Locate beginning of current word
    //            const char* word_end = data->Buf + data->CursorPos;
    //            const char* word_start = word_end;
    //            while (word_start > data->Buf)
    //            {
    //                const char c = word_start[-1];
    //                if (c == ' ' || c == '\t' || c == ',' || c == ';')
    //                    break;
    //                word_start--;
    //            }

    //            // Build a list of candidates
    //            ImVector<const char*> candidates;
    //            for (int i = 0; i < Commands.Size; i++)
    //                if (Strnicmp(Commands[i], word_start, (int)(word_end - word_start)) == 0)
    //                    candidates.push_back(Commands[i]);

    //            if (candidates.Size == 0)
    //            {
    //                // No match
    //                AddLog("No match for \"%.*s\"!\n", (int)(word_end - word_start), word_start);
    //            }
    //            else if (candidates.Size == 1)
    //            {
    //                // Single match. Delete the beginning of the word and replace it entirely so we've got nice casing.
    //                data->DeleteChars((int)(word_start - data->Buf), (int)(word_end - word_start));
    //                data->InsertChars(data->CursorPos, candidates[0]);
    //                data->InsertChars(data->CursorPos, " ");
    //            }
    //            else
    //            {
    //                // Multiple matches. Complete as much as we can..
    //                // So inputing "C"+Tab will complete to "CL" then display "CLEAR" and "CLASSIFY" as matches.
    //                int match_len = (int)(word_end - word_start);
    //                for (;;)
    //                {
    //                    int c = 0;
    //                    bool all_candidates_matches = true;
    //                    for (int i = 0; i < candidates.Size && all_candidates_matches; i++)
    //                        if (i == 0)
    //                            c = toupper(candidates[i][match_len]);
    //                        else if (c == 0 || c != toupper(candidates[i][match_len]))
    //                            all_candidates_matches = false;
    //                    if (!all_candidates_matches)
    //                        break;
    //                    match_len++;
    //                }

    //                if (match_len > 0)
    //                {
    //                    data->DeleteChars((int)(word_start - data->Buf), (int)(word_end - word_start));
    //                    data->InsertChars(data->CursorPos, candidates[0], candidates[0] + match_len);
    //                }

    //                // List matches
    //                AddLog("Possible matches:\n");
    //                for (int i = 0; i < candidates.Size; i++)
    //                    AddLog("- %s\n", candidates[i]);
    //            }

    //            break;
    //        }
    //        case ImGuiInputTextFlags_CallbackHistory:
    //        {
    //            // Example of HISTORY
    //            const int prev_history_pos = HistoryPos;
    //            if (data->EventKey == ImGuiKey_UpArrow)
    //            {
    //                if (HistoryPos == -1)
    //                    HistoryPos = History.Size - 1;
    //                else if (HistoryPos > 0)
    //                    HistoryPos--;
    //            }
    //            else if (data->EventKey == ImGuiKey_DownArrow)
    //            {
    //                if (HistoryPos != -1)
    //                    if (++HistoryPos >= History.Size)
    //                        HistoryPos = -1;
    //            }

    //            // A better implementation would preserve the data on the current input line along with cursor position.
    //            if (prev_history_pos != HistoryPos)
    //            {
    //                const char* history_str = (HistoryPos >= 0) ? History[HistoryPos] : "";
    //                data->DeleteChars(0, data->BufTextLen);
    //                data->InsertChars(0, history_str);
    //            }
    //        }
    //        }
    //        return 0;
    //    }

    //    void ExecCommand(const char* command_line)
    //    {
    //        AddLog("# %s\n", command_line);

    //        // Insert into history. First find match and delete it so it can be pushed to the back.
    //        // This isn't trying to be smart or optimal.
    //        HistoryPos = -1;
    //        for (int i = History.Size - 1; i >= 0; i--)
    //            if (Stricmp(History[i], command_line) == 0)
    //            {
    //                free(History[i]);
    //                History.erase(History.begin() + i);
    //                break;
    //            }
    //        History.push_back(Strdup(command_line));

    //        // Process command
    //        if (Stricmp(command_line, "CLEAR") == 0)
    //        {
    //            ClearLog();
    //        }
    //        else if (Stricmp(command_line, "HELP") == 0)
    //        {
    //            AddLog("Commands:");
    //            for (int i = 0; i < Commands.Size; i++)
    //                AddLog("- %s", Commands[i]);
    //        }
    //        else if (Stricmp(command_line, "HISTORY") == 0)
    //        {
    //            int first = History.Size - 10;
    //            for (int i = first > 0 ? first : 0; i < History.Size; i++)
    //                AddLog("%3d: %s\n", i, History[i]);
    //        }
    //        else
    //        {
    //            AddLog("Unknown command: '%s'\n", command_line);
    //        }

    //        // On command input, we scroll to bottom even if AutoScroll==false
    //        ScrollToBottom = true;
    //    }

    //    std::shared_ptr<spdlog::logger> spdLogger = nullptr;
    //};

    //Console console;

    //class ConsoleSink : public spdlog::sinks::base_sink<std::mutex>
    //{
    //public:
    //    ConsoleSink(Console& console) : console(console) {};
    //    virtual ~ConsoleSink() {};

    //    ConsoleSink(ConsoleSink&) = delete;
    //    ConsoleSink(const ConsoleSink&) = delete;
    //    ConsoleSink& operator=(ConsoleSink&) = delete;
    //    ConsoleSink& operator=(const ConsoleSink&) = delete;

    //protected:

    //    void sink_it_(const spdlog::details::log_msg& msg) override
    //    {
    //        ConsoleLog log = {};
    //        log.message = msg.payload;

    //        console.AddLog(log.message.c_str());

    //        //flush_();
    //    }

    //    void flush_() override
    //    {
    //        //console.AddLog(message);
    //    }

    //private:

    //    Console& console;
    //};

    //bool CreateConsoleLogger()
    //{
    //    std::string logsDirectory = "Logs";
    //    if (!std::filesystem::exists(logsDirectory))
    //    {
    //        std::filesystem::create_directories(logsDirectory);
    //    }

    //    std::vector<spdlog::sink_ptr> sinks =
    //    {
    //        std::make_shared<spdlog::sinks::stdout_color_sink_mt>(),
    //        std::make_shared<ConsoleSink>(console),
    //    };

    //    sinks[0]->set_pattern("%^[%Y-%m-%d %T][%n][%l]%v%$");
    //    sinks[1]->set_pattern("..");

    //    auto colorSink = static_cast<spdlog::sinks::stdout_color_sink_mt*>(sinks[0].get());
    //    colorSink->set_color(spdlog::level::trace, FOREGROUND_BLUE);
    //    colorSink->set_color(spdlog::level::info, std::numeric_limits<uint16_t>::max());
    //    colorSink->set_color(spdlog::level::warn, FOREGROUND_RED | FOREGROUND_GREEN);
    //    colorSink->set_color(spdlog::level::err, FOREGROUND_RED);

    //    console.spdLogger = std::make_shared<spdlog::logger>("Console", sinks.begin(), sinks.end());
    //    console.spdLogger->set_level(spdlog::level::trace);
    //    spdlog::register_logger(console.spdLogger);

    //    GLogger = (Logger*)console.spdLogger.get();

    //    return true;
    //}

    //static int TextEditCallbackStub(ImGuiInputTextCallbackData* data)
    //{
    //    Console* console = (Console*)data->UserData;
    //    return console->TextEditCallback(data);
    //}

    //void HorizonEditor::DrawConsoleWindow(bool* open)
    //{
    //    if (!ImGui::Begin("Console", open))
    //    {
    //        ImGui::End();
    //        return;
    //    }

    //    if (ImGui::SmallButton("Clear"))
    //    {
    //        console.ClearLog();
    //    }
    //    ImGui::SameLine();

    //    ImGui::Separator();

    //    // Reserve enough left-over height for 1 separator + 1 input text
    //    const float footer_height_to_reserve = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
    //    ImGui::BeginChild("ScrollingRegion", ImVec2(0, -footer_height_to_reserve), false, ImGuiWindowFlags_HorizontalScrollbar);
    //    if (ImGui::BeginPopupContextWindow())
    //    {
    //        if (ImGui::Selectable("Clear")) console.ClearLog();
    //        ImGui::EndPopup();
    //    }

    //    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 1)); // Tighten spacing

    //    for (int i = 0; i < console.Items.Size; i++)
    //    {
    //        const char* item = console.Items[i];
    //        if (!console.Filter.PassFilter(item))
    //            continue;

    //        // Normally you would store more information in your item than just a string.
    //        // (e.g. make Items[] an array of structure, store color/type etc.)
    //        ImVec4 color;
    //        bool has_color = false;
    //        if (strstr(item, "[error]")) { color = ImVec4(1.0f, 0.4f, 0.4f, 1.0f); has_color = true; }
    //        else if (strncmp(item, "# ", 2) == 0) { color = ImVec4(1.0f, 0.8f, 0.6f, 1.0f); has_color = true; }
    //        if (has_color)
    //            ImGui::PushStyleColor(ImGuiCol_Text, color);
    //        ImGui::TextUnformatted(item);
    //        if (has_color)
    //            ImGui::PopStyleColor();
    //    }

    //    if (console.ScrollToBottom || (console.AutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()))
    //        ImGui::SetScrollHereY(1.0f);
    //    console.ScrollToBottom = false;

    //    ImGui::PopStyleVar();
    //    ImGui::EndChild();
    //    ImGui::Separator();

    //    // Command-line
    //    bool reclaim_focus = false;
    //    ImGuiInputTextFlags input_text_flags = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackCompletion | ImGuiInputTextFlags_CallbackHistory;
    //    if (ImGui::InputText("Input", console.inputBuffer, IM_ARRAYSIZE(console.inputBuffer), input_text_flags, &TextEditCallbackStub, (void*)&console))
    //    {
    //        char* s = console.inputBuffer;
    //        console.Strtrim(s);
    //        if (s[0])
    //        {
    //            console.ExecCommand(s);
    //        }
    //        strcpy(s, "");
    //        reclaim_focus = true;
    //    }
    //    // Auto-focus on window apparition
    //    ImGui::SetItemDefaultFocus();

    //    if (reclaim_focus)
    //    {
    //        ImGui::SetKeyboardFocusHere(-1); // Auto focus previous widget
    //    }
    //    ImGui::End();
    //}

    //std::unordered_map<std::string, std::function<void(const char*, const char* name, void*)>> uiCreator;
    //std::vector<std::pair<std::string, bool>> g_editor_node_state_array;
    //int                                       g_node_depth = -1;
    //bool inited = false;

    //void UIInit()
    //{
    //    using namespace Horizon;

    //    uiCreator["bool"] = [](const char* lable, const char* name, void* value)
    //        {
    //            ImGui::AlignTextToFramePadding();
    //            ImGui::TextUnformatted(name);
    //            ImGui::NextColumn();
    //            ImGui::PushItemWidth(-1);
    //            ImGui::Checkbox(lable, static_cast<bool*>(value));
    //            ImGui::PopItemWidth();
    //            ImGui::NextColumn();
    //        };

    //    uiCreator["int"] = [](const char* lable, const char* name, void* value)
    //        {
    //            ImGui::AlignTextToFramePadding();
    //            ImGui::TextUnformatted(name);
    //            ImGui::NextColumn();
    //            ImGui::PushItemWidth(-1);
    //            ImGui::DragInt(lable, static_cast<int*>(value));
    //            ImGui::PopItemWidth();
    //            ImGui::NextColumn();
    //        };

    //    uiCreator["unsigned int"] = [](const char* lable, const char* name, void* value)
    //        {
    //            ImGui::AlignTextToFramePadding();
    //            ImGui::TextUnformatted(name);
    //            ImGui::NextColumn();
    //            ImGui::PushItemWidth(-1);
    //            ImGui::DragInt(lable, static_cast<int*>(value));
    //            ImGui::PopItemWidth();
    //            ImGui::NextColumn();
    //        };

    //    uiCreator["float"] = [](const char* lable, const char* name, void* value)
    //        {
    //            ImGui::AlignTextToFramePadding();
    //            ImGui::TextUnformatted(name);
    //            ImGui::NextColumn();
    //            ImGui::PushItemWidth(-1);
    //            ImGui::DragFloat(lable, static_cast<float*>(value));
    //            ImGui::PopItemWidth();
    //            ImGui::NextColumn();
    //        };

    //    uiCreator["struct glm::vec<3,float,0>"] = [](const char* lable, const char* name, void* value)
    //        {
    //            ImGui::AlignTextToFramePadding();
    //            ImGui::TextUnformatted(name);
    //            ImGui::NextColumn();
    //            ImGui::PushItemWidth(-1);
    //            ImGui::DragFloat3(lable, static_cast<float*>(value));
    //            ImGui::PopItemWidth();
    //            ImGui::NextColumn();
    //        };

    //    uiCreator["class std::basic_string<char,struct std::char_traits<char>,class std::allocator<char> >"] = [](const char* lable, const char* name, void* value)
    //        {
    //            ImGui::AlignTextToFramePadding();
    //            ImGui::TextUnformatted(name);
    //            ImGui::NextColumn();
    //            ImGui::PushItemWidth(-1);
    //            ImGui::Text(lable, static_cast<std::string*>(value)->c_str());
    //            /*if (ImGui::InputText(lable, static_cast<std::string*>(value)->c_str(), 256))
    //            {

    //            }*/
    //            ImGui::PopItemWidth();
    //            ImGui::NextColumn();
    //        };
    //}

    void BeginDockSpace()
    {
        static bool dockSpaceOpen = true;

        // Imgui dock node flags.
        static ImGuiDockNodeFlags dockNodeflags = ImGuiDockNodeFlags_PassthruCentralNode;

        // Imgui window flags.
        ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoDocking;

        static bool isFullscreenPersistant = true;
        bool isFullscreen = isFullscreenPersistant;
        if (isFullscreen)
        {
            ImGuiViewport* viewport = ImGui::GetMainViewport();
            ImGui::SetNextWindowPos(viewport->Pos);
            ImGui::SetNextWindowSize(viewport->Size);
            ImGui::SetNextWindowViewport(viewport->ID);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
            windowFlags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
            windowFlags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
        }

        windowFlags |= ImGuiWindowFlags_NoBackground;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        // When using ImGuiDockNodeFlags_PassthruDockspace, DockSpace() will render our background and handle the pass-thru hole, so we ask Begin() to not render a background.
        ImGui::Begin("Dockspace", &dockSpaceOpen, windowFlags);

        ImGui::PopStyleVar();

        if (isFullscreen)
        {
            ImGui::PopStyleVar(2);
        }

        // Set min width
        ImGuiIO& io = ImGui::GetIO();
        ImGuiStyle& style = ImGui::GetStyle();
        float minWinSizeX = style.WindowMinSize.x;
        style.WindowMinSize.x = 300.0f;
        if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
        {
            ImGuiID dockSpaceID = ImGui::GetID("MyDockSpace");
            ImGui::DockSpace(dockSpaceID, ImVec2(0.0f, 0.0f), dockNodeflags);
        }
        style.WindowMinSize.x = minWinSizeX;
    }

    void EndDockSpace()
    {
        ImGui::End();
    }

    //void DrawMenuBar()
    //{
    //    if (ImGui::BeginMainMenuBar())
    //    {
    //        if (ImGui::BeginMenu("File"))
    //        {
    //            //ShowExampleMenuFile();
    //            ImGui::EndMenu();
    //        }
    //        if (ImGui::BeginMenu("Edit"))
    //        {
    //            if (ImGui::MenuItem("Undo", "CTRL+Z")) {}
    //            if (ImGui::MenuItem("Redo", "CTRL+Y", false, false)) {}  // Disabled item
    //            ImGui::Separator();
    //            if (ImGui::MenuItem("Cut", "CTRL+X")) {}
    //            if (ImGui::MenuItem("Copy", "CTRL+C")) {}
    //            if (ImGui::MenuItem("Paste", "CTRL+V")) {}
    //            ImGui::EndMenu();
    //        }
    //        ImGui::EndMainMenuBar();
    //    }
    //}

    float HorizonEditor::GetSnapValue()
    {
        switch (gizmoOperationType)
        {
        case ImGuizmo::OPERATION::TRANSLATE: return 5.0f; break;
        case ImGuizmo::OPERATION::ROTATE: return 10.0f; break;
        case ImGuizmo::OPERATION::SCALE: return 0.1f; break;
        }
        return 0.0f;
    }

    //void HorizonEditor::DrawOverlay()
    //{
    //    static int corner = 0;
    //    ImGuiIO& io = ImGui::GetIO();
    //    ImGuiWindowFlags windowFags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;
    //    if (corner != -1)
    //    {
    //        const float padding = 10.0f;
    //        const ImGuiViewport* viewport = ImGui::GetMainViewport();
    //        ImVec2 workPos = viewport->WorkPos; // Use work area to avoid menu-bar/task-bar, if any!
    //        ImVec2 workSize = viewport->WorkSize;
    //        ImVec2 windowPos, windowPosPivot;
    //        windowPos.x = (corner & 1) ? (workPos.x + workSize.x - padding) : (workPos.x + padding);
    //        windowPos.y = (corner & 2) ? (workPos.y + workSize.y - padding) : (workPos.y + padding);
    //        windowPosPivot.x = (corner & 1) ? 1.0f : 0.0f;
    //        windowPosPivot.y = (corner & 2) ? 1.0f : 0.0f;
    //        ImGui::SetNextWindowPos(windowPos, ImGuiCond_Always, windowPosPivot);
    //        ImGui::SetNextWindowViewport(viewport->ID);
    //        windowFags |= ImGuiWindowFlags_NoMove;
    //    }
    //    ImGui::SetNextWindowBgAlpha(0.35f); // Transparent background
    //    static bool open = true;
    //    if (ImGui::Begin("Overlay", &open, windowFags))
    //    {
    //        ImGui::Text(HORIZON_ENGINE_NAME);
    //        ImGui::Separator();
    //        ImGui::Text("FPS: %.1f (%.2f ms/frame)", ImGui::GetIO().Framerate, (1000.0f / ImGui::GetIO().Framerate));

    //        static RenderBackendTextureHandle renderDocLogoTexture = RenderBackendTextureHandle::Null;
    //        if (renderDocLogoTexture == RenderBackendTextureHandle::Null)
    //        {
    //            renderDocLogoTexture = LoadTextureFromFile(GRenderBackend, "../../../Source/Editor/Plugins/RenderDoc/Resources/renderdoc_logo.png", false, false);
    //        }

    //        if (UI::ImageButton("##RenderDocTriggerCapture", renderDocLogoTexture.ToUnit64(), ImVec2(25, 25)))
    //        {
    //            RenderDocPluginTriggerCapture();
    //        }

    //        // TODO
    //        DrawViewSettings();
    //    }
    //    ImGui::End();
    //}

    void HorizonEditor::DrawProfilerWindow(bool* open)
    {
        RenderBackendGPUProfiler* gpuProfiler = engine->GetSubsystem<RenderSystem>()->gpuProfiler;
        if (ImGui::Begin("Profiler", open))
        {
            ImGui::Text("FPS: %.1f (%.4f ms/frame)", ImGui::GetIO().Framerate, (1000.0f / ImGui::GetIO().Framerate));

            if (ImGui::CollapsingHeader("CPU Profiler", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::Text("CPU Frametime: %.4f ms", 1000.0f * deltaTimeInSeconds);
            }

            if (ImGui::CollapsingHeader("GPU Profiler", ImGuiTreeNodeFlags_DefaultOpen))
            {
                for (uint32 regionIndex = 0; regionIndex < gpuProfiler->GetRegionCount(); regionIndex++)
                {
                    ImGui::Text("%s: %.4f ms", gpuProfiler->GetRegionName(regionIndex), gpuProfiler->GetRegionTime(regionIndex));
                }
            }
        }
        ImGui::End();
    }

    void DrawEntityNodeUI(EntityHandle entity)
    {
        EntityHandle selectedEntity = HorizonEditor::GetInstance()->GetSelectedEntity();

        EntityManager* entityManager = HorizonEditor::GetInstance()->GetEditorSceneManager()->GetActiveScene()->GetEntityManager();
        const SceneHierarchyComponent& hierarchy = entityManager->GetComponent<SceneHierarchyComponent>(entity);
        ImGuiTreeNodeFlags flags = ((selectedEntity != EntityHandle::Null && selectedEntity == entity) ? ImGuiTreeNodeFlags_Selected : 0) | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
        if (hierarchy.numChildren == 0)
        {
            flags |= ImGuiTreeNodeFlags_Leaf;
        }
        bool opened = ImGui::TreeNodeEx((void*)(uint64)entity, flags, entityManager->GetComponent<NameComponent>(entity).GetName());
        if (ImGui::IsItemClicked())
        {
            HorizonEditor::GetInstance()->SetSelectedEntity(entity);
        }

        if (opened)
        {
            EntityHandle currentEntity = hierarchy.firstChild;
            for (uint32 i = 0; i < hierarchy.numChildren; i++)
            {
                DrawEntityNodeUI(currentEntity);
                currentEntity = entityManager->GetComponent<SceneHierarchyComponent>(currentEntity).next;
            }
            ImGui::TreePop();
        }

        bool deleted = false;
        if (ImGui::BeginPopupContextItem())
        {
            if (ImGui::MenuItem("Delete Entity"))
            {
                deleted = true;
            }
            ImGui::EndPopup();
        }

        if (deleted)
        {
            entityManager->DestroyEntity(entity);
            if (selectedEntity == entity)
            {
                selectedEntity = EntityHandle::Null;
            }
        }
    }

    void HorizonEditor::DrawSceneHierarchyWindow(bool* open)
    {
        if (ImGui::Begin("Scene Hierarchy", open))
        {
            ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_DefaultOpen;
            if (ImGui::TreeNodeEx((void*)(uint64)564788, flags, editorSceneManager->GetActiveScene()->GetName().c_str()))
            {
                EntityHandle selectedEntity = HorizonEditor::GetInstance()->GetSelectedEntity();
                auto entityManager = editorSceneManager->GetActiveScene()->GetEntityManager();
                entityManager->Get()->each([&](EntityHandle entity)
                    {
                        if (entityManager->GetComponent<SceneHierarchyComponent>(entity).parent == EntityHandle::Null)
                        {
                            DrawEntityNodeUI(entity);
                        }
                    });
                ImGui::TreePop();
            }

            if (ImGui::IsMouseDown(0) && ImGui::IsWindowHovered())
            {
                HorizonEditor::GetInstance()->SetSelectedEntity(EntityHandle::Null);
            }

            // Right-click on blank space
            if (ImGui::BeginPopupContextWindow())
            {
                if (ImGui::MenuItem("Create Empty Entity"))
                {
                    auto newEntity = editorSceneManager->GetActiveScene()->CreateEntity("Empty Entity");
                    editorSceneManager->GetActiveScene()->GetEntityManager()->AddComponent<TransformComponent>(newEntity);
                    editorSceneManager->GetActiveScene()->GetEntityManager()->AddComponent<SceneHierarchyComponent>(newEntity);
                }
                ImGui::EndPopup();
            }
        }
        ImGui::End();
    }

    void HorizonEditor::OnDrawUIEx()
    {
        // Draw gizmos
        {
            auto windowPos = ImGui::GetWindowPos();
            //auto viewportSize = ImGui::GetContentRegionAvail();
            Vector2f viewportSize = { swapChainWidth, swapChainHeight };

            if (selectedEntity && gizmoOperationType != -1)
            {
                ImGuizmo::SetOrthographic(false);
                ImGuizmo::SetDrawlist();
                ImGuizmo::SetRect(windowPos.x, windowPos.y, viewportSize.x, viewportSize.y);

                bool snap = Input::GetKeyDown(KeyCode::LeftControl);
                float snapValue = GetSnapValue();
                float snapValues[3] = { snapValue, snapValue, snapValue };

                // Editor camera
                Matrix4x4f cameraProjection = projectionMatrix_deprecated;
                Matrix4x4f cameraView = viewMatrix_deprecated;

                // Entity transform
                static Matrix4x4f transformMatrix = Matrix4x4f(1.0f);
                TransformComponent& transformComponent = editorSceneManager->GetActiveScene()->GetEntityManager()->GetComponent<TransformComponent>(selectedEntity);
                transformMatrix = transformComponent.localToWorldMatrix;

                //float deltaMatrix[16];

                ImVec4 deltaTranslation; ImVec4 deltaRotation; ImVec4 deltaScale;
                ImGuizmo::Manipulate(glm::value_ptr(cameraView),
                    glm::value_ptr(cameraProjection),
                    (ImGuizmo::OPERATION)gizmoOperationType,
                    ImGuizmo::LOCAL,
                    glm::value_ptr(transformMatrix),
                    &deltaTranslation, &deltaRotation, &deltaScale,
                    nullptr,
                    snap ? snapValues : nullptr);

                /*static Quaternion zUpQuat = glm::rotate(glm::quat(), Math::DegreesToRadians(90.0), Vector3f(1.0, 0.0, 0.0));
                static Matrix4x4f preTransform = Math::Compose(Vector3f(0.0f, 0.0f, 0.0f), zUpQuat, Vector3f(1.0f, 1.0f, 1.0f));
                ImGuizmo::DrawGrid(glm::value_ptr(cameraView),
                    glm::value_ptr(cameraProjection),
                    glm::value_ptr(preTransform),
                    10.0f);*/

                if (ImGuizmo::IsUsing())
                {
                    TransformComponent& _transformComponent = editorSceneManager->GetActiveScene()->GetEntityManager()->GetComponent<TransformComponent>(selectedEntity);
                    _transformComponent.position += Vector3f(deltaTranslation.x, deltaTranslation.y, deltaTranslation.z);
                    //editorSceneManager->GetActiveScene()->GetEntityManager()->ReplaceComponent<TransformComponent>(selectedEntity, transformComponent);
                }
            }
        }
    }

    void HorizonEditor::DrawSceneViewWindow()
    {
        static bool open = true;

        ImGuiIO& io = ImGui::GetIO();

        ImGui::SetNextWindowBgAlpha(0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0, 0 });

        ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;

        ImGui::Begin("SceneView", &open, flags);

        static const char* viewModes[] = { "Lighting", "Wireframe", "Illuminance", "Depth", "World Space Normal", "Primitive ID", "Material ID", "Motion Vectors", "Ambient Occlusion", "Screen Space Shadow Mask", "Surfel GI Surfel", "Surfel GI Heatmap" };
        static int currentViewModeIndex = 0;
        ImGui::Combo("##ViewMode", &currentViewModeIndex, viewModes, IM_ARRAYSIZE(viewModes));
        currentDebugVisualizationMode = (SceneViewDebugVisualizationMode)currentViewModeIndex;

        static RenderBackendTextureHandle renderDocLogoTexture = RenderBackendTextureHandle::Null;
        if (renderDocLogoTexture == RenderBackendTextureHandle::Null)
        {
            renderDocLogoTexture = LoadTextureFromFile(renderBackend, nullptr, "../../../Source/HorizonEditor/Plugins/RenderDoc/Resources/renderdoc_logo.png", false, false);
        }
        if (ImGui::ImageButtonEx(ImGui::GetID("##RenderDocCapture"), renderDocLogoTexture.ToUnit64(), ImVec2(25, 25), ImVec2(0, 0), ImVec2(1, 1), ImVec4(0, 0, 0, 0), ImVec4(1, 1, 1, 1)))
        {
            RenderDocPluginTriggerCapture();
        }

        ImVec2 contentRegionAvail = ImGui::GetContentRegionAvail();
        viewportSize = Vector2f(contentRegionAvail.x, contentRegionAvail.y);

        ImGui::Image(previewTexture->GetHandle().ToUnit64(), ImVec2(viewportSize.x, viewportSize.y), ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f));

        ImGui::End();

        ImGui::PopStyleVar();
    }

   void HorizonEditor::DrawRenderSettingsWindow(bool* open)
    {
        if (ImGui::Begin("Render Settings", open))
        {
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
            ImGui::Columns(2);
            ImGui::Separator();

            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Rendering Mode");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);

            static const char* renderModeNames[] = { "Rasterization", "Hybrid Rendering", "Path Tracing (Real-Time)", "Path Tracing (Reference)" };
            int renderMode = (int)renderSettings.renderMode;
            ImGui::Combo("##Rendering Mode", &renderMode, renderModeNames, IM_ARRAYSIZE(renderModeNames));
            renderSettings.renderMode = (RenderMode)renderMode;

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
                if (ImGui::Checkbox("##FixedPreExposure", &renderSettings.enableFixedPreExposure))
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
                ImGui::TextUnformatted("Indirect Lighting Color");
                ImGui::NextColumn();
                ImGui::PushItemWidth(-1);
                if (ImGui::ColorEdit3("##indirectLightingColor", &renderSettings.globalIlluminationSettings.indirectLightingColor.x))
                {

                }
                ImGui::PopItemWidth();
                ImGui::NextColumn();

                ImGui::AlignTextToFramePadding();
                ImGui::TextUnformatted("Indirect Lighting Intensity");
                ImGui::NextColumn();
                ImGui::PushItemWidth(-1);
                if (ImGui::DragFloat("##indirectLightingIntensity", &renderSettings.globalIlluminationSettings.indirectLightingIntensity, 0.01f, 0.0f, 1000.0f))
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

                const char* items[] = { "None", "Shadow Map", "Virtual Shadow Map", "Ray Tracing Shadows" };
                int item = (int)renderSettings.shadowsTechnique;
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
                int item = int(renderSettings.reflectionsTechnique);
                ImGui::Combo("##ReflectionsTechnique", &item, items, IM_ARRAYSIZE(items));
                renderSettings.reflectionsTechnique = (ReflectionsTechnique)item;

                ImGui::PopItemWidth();
                ImGui::NextColumn();

                ImGui::AlignTextToFramePadding();
                ImGui::TextUnformatted("Denosing");
                ImGui::NextColumn();
                ImGui::PushItemWidth(-1);
                if (ImGui::Checkbox("##SSRDenosing", &renderSettings.ssrSettings.enableDenoising))
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
                renderSettings.ssrSettings.quality = (ScreenSpaceReflectionsQuality)item2;

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

            if (ImGui::CollapsingHeader("Super Sampling", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
                ImGui::Columns(2);
                ImGui::Separator();

                ImGui::AlignTextToFramePadding();
                ImGui::TextUnformatted("Technique");
                ImGui::NextColumn();
                ImGui::PushItemWidth(-1);

                const char* items[] = { "None", "AMD FSR", "NVIDIA DLSS" };
                int item = int(renderSettings.superSamplingSettings.superSamplingTechnique);
                ImGui::Combo("##SuperResolutionTechnique", &item, items, IM_ARRAYSIZE(items));
                renderSettings.superSamplingSettings.superSamplingTechnique = SuperSamplingTechnique(item);

                ImGui::PopItemWidth();
                ImGui::NextColumn();

                switch (item)
                {
                case uint32(SuperSamplingTechnique::FSR):
                {
                    ImGui::AlignTextToFramePadding();
                    ImGui::TextUnformatted("Quality Mode");
                    ImGui::NextColumn();
                    ImGui::PushItemWidth(-1);

                    static const char* fsrQualityModeNames[] = { "Off", "Quality", "Balanced", "Performance", "Ultra Performance", "Custom" };
                    int fsrQualityModeNameIndex = int(renderSettings.superSamplingSettings.qualityMode);
                    ImGui::Combo("##FSRQualityMode", &fsrQualityModeNameIndex, fsrQualityModeNames, IM_ARRAYSIZE(fsrQualityModeNames));
                    renderSettings.superSamplingSettings.qualityMode = uint32(fsrQualityModeNameIndex);
                    ImGui::PopItemWidth();
                    ImGui::NextColumn();

                    //if (renderSettings.superSamplingSettings.qualityMode == FidelityFXSuperResolution2QualityMode::Custom)
                    {
                        ImGui::AlignTextToFramePadding();
                        ImGui::TextUnformatted("Render Resolution Percentage");
                        ImGui::NextColumn();
                        ImGui::PushItemWidth(-1);

                        int desiredRenderResolutionPercentage = int(renderSettings.superSamplingSettings.desiredRenderResolutionPercentage * 100.0f);
                        if (ImGui::DragInt("##desiredRenderResolutionPercentage", &desiredRenderResolutionPercentage, 1, 25, 100))
                        {
                            renderSettings.superSamplingSettings.desiredRenderResolutionPercentage = 0.01f * desiredRenderResolutionPercentage;
                        }

                        ImGui::PopItemWidth();
                        ImGui::NextColumn();
                    }

                    ImGui::AlignTextToFramePadding();
                    ImGui::TextUnformatted("Frame Interpolation");
                    ImGui::NextColumn();
                    ImGui::PushItemWidth(-1);
                    if (ImGui::Checkbox("##FrameInterpolation", &renderSettings.superSamplingSettings.enableFrameInterpolation))
                    {

                    }
                    ImGui::PopItemWidth();
                    ImGui::NextColumn();
                } break;
                case uint32(SuperSamplingTechnique::DLSS):
                {
                    ImGui::AlignTextToFramePadding();
                    ImGui::TextUnformatted("Quality Mode");
                    ImGui::NextColumn();
                    ImGui::PushItemWidth(-1);

                    static const char* dlssQualityModeNames[] = { "Off", "Auto", "Quality", "Balanced", "Performance", "Ultra Performance", "Ultra Quality" };
                    static int dlssQualityModeNameIndex = 1;
                    ImGui::Combo("##DLSSQualityMode", &dlssQualityModeNameIndex, dlssQualityModeNames, IM_ARRAYSIZE(dlssQualityModeNames));
                    renderSettings.superSamplingSettings.qualityMode = uint32(dlssQualityModeNameIndex);
                    ImGui::PopItemWidth();
                    ImGui::NextColumn();

                    {
                        ImGui::AlignTextToFramePadding();
                        ImGui::TextUnformatted("Render Resolution Percentage");
                        ImGui::NextColumn();
                        ImGui::PushItemWidth(-1);

                        int desiredRenderResolutionPercentage = int(renderSettings.superSamplingSettings.desiredRenderResolutionPercentage * 100.0f);
                        if (ImGui::DragInt("##desiredRenderResolutionPercentage", &desiredRenderResolutionPercentage, 1, 25, 100))
                        {
                            renderSettings.superSamplingSettings.desiredRenderResolutionPercentage = 0.01f * float(desiredRenderResolutionPercentage);
                        }

                        ImGui::PopItemWidth();
                        ImGui::NextColumn();
                    }

                    ImGui::AlignTextToFramePadding();
                    ImGui::TextUnformatted("Frame Interpolation");
                    ImGui::NextColumn();
                    ImGui::PushItemWidth(-1);
                    if (ImGui::Checkbox("##FrameInterpolation", &renderSettings.superSamplingSettings.enableFrameInterpolation))
                    {

                    }
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
#if 0
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
#endif
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
#if 0
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
#endif
                if (ImGui::TreeNode("Lens Flare"))
                {
                    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
                    ImGui::Columns(2);
                    ImGui::Separator();

                    ImGui::AlignTextToFramePadding();
                    ImGui::TextUnformatted("Intensity");
                    ImGui::NextColumn();
                    ImGui::PushItemWidth(-1);
                    if (ImGui::DragFloat("##lensFlareIntensity", &renderSettings.postProcessingSettings.lensFlareIntensity, 0.001f, 0.0f, 10.0f))
                    {

                    }
                    ImGui::PopItemWidth();
                    ImGui::NextColumn();

                    ImGui::AlignTextToFramePadding();
                    ImGui::TextUnformatted("Halo Intensity");
                    ImGui::NextColumn();
                    ImGui::PushItemWidth(-1);
                    if (ImGui::DragFloat("##lensFlareHaloIntensity", &renderSettings.postProcessingSettings.lensFlareHaloIntensity, 0.001f, 0.0f, 10.0f))
                    {

                    }
                    ImGui::PopItemWidth();
                    ImGui::NextColumn();

                    ImGui::AlignTextToFramePadding();
                    ImGui::TextUnformatted("Halo Width");
                    ImGui::NextColumn();
                    ImGui::PushItemWidth(-1);
                    if (ImGui::DragFloat("##lensFlareHaloWidth", &renderSettings.postProcessingSettings.lensFlareHaloWidth, 0.001f, 0.0f, 10.0f))
                    {

                    }
                    ImGui::PopItemWidth();
                    ImGui::NextColumn();

                    ImGui::AlignTextToFramePadding();
                    ImGui::TextUnformatted("Halo Chromatic Aberration Offset");
                    ImGui::NextColumn();
                    ImGui::PushItemWidth(-1);
                    if (ImGui::DragFloat("##lensFlareHaloChromaticAberrationOffset", &renderSettings.postProcessingSettings.lensFlareHaloChromaticAberrationOffset, 0.001f, 0.0f, 10.0f))
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
                    int item = (int)renderSettings.postProcessingSettings.exposureMethod;

                    ImGui::AlignTextToFramePadding();
                    ImGui::TextUnformatted("Exposure Method");
                    ImGui::NextColumn();
                    ImGui::PushItemWidth(-1);
                    if (ImGui::Combo("##ExposureMethod", &item, items, IM_ARRAYSIZE(items)))
                    {
                        renderSettings.postProcessingSettings.exposureMethod = (ExposureMethod)item;
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
                    ImGui::TextUnformatted("Lower Percentage");
                    ImGui::NextColumn();
                    ImGui::PushItemWidth(-1);
                    if (ImGui::DragFloat("##autoExposureHistogramLowerPercentage", &renderSettings.postProcessingSettings.autoExposureHistogramLowerPercentage, 0.1f, 0.0f, 100.0f))
                    {

                    }
                    ImGui::PopItemWidth();
                    ImGui::NextColumn();

                    ImGui::AlignTextToFramePadding();
                    ImGui::TextUnformatted("Higher Percentage");
                    ImGui::NextColumn();
                    ImGui::PushItemWidth(-1);
                    if (ImGui::DragFloat("##autoExposureHistogramHigherPercentage", &renderSettings.postProcessingSettings.autoExposureHistogramHigherPercentage, 0.1f, 0.0f, 100.0f))
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
#if 0
                if (ImGui::TreeNode("Local Exposure"))
                {
                    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
                    ImGui::Columns(2);
                    ImGui::Separator();

                    ImGui::AlignTextToFramePadding();
                    ImGui::TextUnformatted("Enable");
                    ImGui::NextColumn();
                    ImGui::PushItemWidth(-1);
                    if (ImGui::Checkbox("##LocalExposure", &renderSettings.postProcessingSettings.localExposureEnabled))
                    {

                    }
                    ImGui::PopItemWidth();
                    ImGui::NextColumn();

                    ImGui::AlignTextToFramePadding();
                    ImGui::TextUnformatted("Shadows");
                    ImGui::NextColumn();
                    ImGui::PushItemWidth(-1);
                    if (ImGui::DragFloat("##LocalExposureShadows", &renderSettings.postProcessingSettings.localExposureShadows, 0.01f, 0.0f, 4.0f))
                    {

                    }
                    ImGui::PopItemWidth();
                    ImGui::NextColumn();

                    ImGui::AlignTextToFramePadding();
                    ImGui::TextUnformatted("Highlights");
                    ImGui::NextColumn();
                    ImGui::PushItemWidth(-1);
                    if (ImGui::DragFloat("##LocalExposureHighlights", &renderSettings.postProcessingSettings.localExposureHighlights, 0.01f, 0.0f, 4.0f))
                    {

                    }
                    ImGui::PopItemWidth();
                    ImGui::NextColumn();

                    ImGui::AlignTextToFramePadding();
                    ImGui::TextUnformatted("Coarsest Mip Level");
                    ImGui::NextColumn();
                    ImGui::PushItemWidth(-1);
                    if (ImGui::DragInt("##LocalExposureCoarsestMipLevel", &renderSettings.postProcessingSettings.localExposureCoarsestMipLevel, 1.0f, 0, 32))
                    {

                    }
                    ImGui::PopItemWidth();
                    ImGui::NextColumn();

                    ImGui::AlignTextToFramePadding();
                    ImGui::TextUnformatted("Display Mip Level");
                    ImGui::NextColumn();
                    ImGui::PushItemWidth(-1);
                    if (ImGui::DragInt("##LocalExposureDisplayMipLevel", &renderSettings.postProcessingSettings.localExposureDisplayMipLevel, 1.0f, 0, 32))
                    {

                    }
                    ImGui::PopItemWidth();
                    ImGui::NextColumn();

                    ImGui::AlignTextToFramePadding();
                    ImGui::TextUnformatted("Exposure Preference Sigma");
                    ImGui::NextColumn();
                    ImGui::PushItemWidth(-1);
                    if (ImGui::DragFloat("##LocalExposurePreferenceSigma", &renderSettings.postProcessingSettings.localExposurePreferenceSigma))
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
#endif
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
#if 0
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
#endif
                if (ImGui::TreeNode("Color Grading"))
                {
                    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
                    ImGui::Columns(2);
                    ImGui::Separator();

                    ImGui::AlignTextToFramePadding();
                    ImGui::TextUnformatted("White Balance");
                    ImGui::NextColumn();
                    ImGui::PushItemWidth(-1);
                    if (ImGui::DragFloat("##WhiteBalance", &renderSettings.postProcessingSettings.whiteBalance, 0.1f, 1000.0f, 25000.0f))
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

    void HorizonEditor::OnDrawUI()
    {
        OPTICK_EVENT();

        BeginDockSpace();

        //if (!inited)
        //{
        //    UIInit();
        //    inited = true;
        //}

        //ImVec2 cursorPos = ImGui::GetCursorPos();
        //cursorPos = { 0, 0 };
        //ImVec2 windowPos = ImGui::GetWindowPos();
        //ImVec2 windowSize = ImGui::GetWindowSize();
        //viewportPos = Vector4f(cursorPos.x + windowPos.x, cursorPos.y + windowPos.y, cursorPos.x + windowPos.x + windowSize.x, cursorPos.y + windowPos.y + windowSize.y);

        if (showSceneHierarchyWindow)
        {
            DrawSceneHierarchyWindow(&showSceneHierarchyWindow);
        }

        if (showInspectorWindow)
        {
            DrawInspectorWindow(&showInspectorWindow);
        }

        if (showProfilerWindow)
        {
            DrawProfilerWindow(&showProfilerWindow);
        }

        //if (showConsoleWindow)
        //{
        //    DrawConsoleWindow(&showConsoleWindow);
        //}

        if (showRenderSettingsWindow)
        {
            DrawRenderSettingsWindow(&showRenderSettingsWindow);
        }

        DrawSceneViewWindow();

        //bool showSceneViewportWindow = true;
        //if (true)
        //{
        //    if (!sceneViewportWindow)
        //    {
        //        sceneViewportWindow = new SceneViewportWindow("DefaultScene", this);
        //    }
        //    sceneViewportWindow->OnImGuiRender(showSceneViewportWindow);
        //}

        //if (true)
        //{
        //    if (!fileBrowserWindow)
        //    {
        //        fileBrowserWindow = new FileBrowserWindow(this, nullptr);
        //    }
        //    fileBrowserWindow->OnImGuiRender();
        //}

        //if (true)
        //{
        //    ImGui::Begin("node editor");
        //    const int hardcoded_node_id = 1;

        //    ImNodes::BeginNodeEditor();

        //    const bool openPopup = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) &&
        //        ImNodes::IsEditorHovered() &&
        //        ImGui::IsMouseReleased(ImGuiMouseButton_Right);

        //    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.f, 8.f));
        //    if (!ImGui::IsAnyItemHovered() && openPopup)
        //    {
        //        ImGui::OpenPopup("Add Node");
        //    }

        //    if (ImGui::BeginPopup("Add Node"))
        //    {
        //        const ImVec2 click_pos = ImGui::GetMousePosOnOpeningCurrentPopup();

        //        if (ImGui::MenuItem("Multiply"))
        //        {
        //            const int node_id = ++current_id;
        //            ImNodes::SetNodeScreenSpacePos(node_id, ImGui::GetMousePos());
        //            ImNodes::SnapNodeToGrid(node_id);
        //            ShadeGraphNode* newNode = new ShadeGraphNode();
        //            newNode->id = node_id;
        //            newNode->type = ShadeGraphFindNodeType("Multiply");
        //            newNode->title = newNode->type->uniqueName;
        //            newNode->Init();
        //            nodes.push_back(newNode);

        //            ImNodes::SetNodeScreenSpacePos(node_id, click_pos);
        //        }

        //        if (ImGui::MenuItem("Sample Texture 2D"))
        //        {
        //            const int node_id = ++current_id;
        //            ImNodes::SetNodeScreenSpacePos(node_id, ImGui::GetMousePos());
        //            ImNodes::SnapNodeToGrid(node_id);
        //            ShadeGraphNode* newNode = new ShadeGraphNode();
        //            newNode->id = node_id;
        //            newNode->type = ShadeGraphFindNodeType("Sample Texture 2D");
        //            newNode->title = newNode->type->uniqueName;
        //            newNode->Init();
        //            nodes.push_back(newNode);

        //            ImNodes::SetNodeScreenSpacePos(node_id, click_pos);
        //        }

        //        if (ImGui::MenuItem("Principled BSDF"))
        //        {
        //            const int node_id = ++current_id;
        //            ImNodes::SetNodeScreenSpacePos(node_id, ImGui::GetMousePos());
        //            ImNodes::SnapNodeToGrid(node_id);
        //            ShadeGraphNode* newNode = new ShadeGraphNode();
        //            newNode->id = node_id;
        //            newNode->type = ShadeGraphFindNodeType("Principled BSDF");
        //            newNode->title = newNode->type->uniqueName;
        //            newNode->Init();
        //            nodes.push_back(newNode);

        //            ImNodes::SetNodeScreenSpacePos(node_id, click_pos);
        //        }

        //        ImGui::EndPopup();
        //    }
        //    ImGui::PopStyleVar();

        //    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
        //    for (ShadeGraphNode* node : nodes)
        //    {
        //        ImNodes::BeginNode(node->id, IM_COL32(43, 101, 43, 255));

        //        ImNodes::BeginNodeTitleBar();
        //        ImGui::TextUnformatted(node->title.c_str());
        //        ImNodes::EndNodeTitleBar();

        //        for (ShadeGraphPinInstance* input : node->inputs)
        //        {
        //            //if ()
        //            {
        //                ImNodes::BeginInputAttribute((node->id << 8) | (input->GetIndexInNode()));
        //                ImGui::TextUnformatted(input->type->name.c_str());
        //                ImNodes::EndInputAttribute();
        //            }
        //            //else // TODO
        //            //{
        //            //    ImNodes::BeginStaticAttribute(node.id << 16);
        //            //    ImGui::PushItemWidth(120.0f);
        //            //    ImGui::DragFloat("value", &node.value, 0.01f);
        //            //    ImGui::PopItemWidth();
        //            //    ImNodes::EndStaticAttribute();
        //            //}
        //        }

        //        for (ShadeGraphPinInstance* output : node->outputs)
        //        {
        //            ImNodes::BeginOutputAttribute((node->id << 24) | (output->GetIndexInNode()));
        //            const float text_width = ImGui::CalcTextSize("output").x;
        //            ImGui::Indent(120.f + ImGui::CalcTextSize("value").x - text_width);
        //            ImGui::TextUnformatted(output->type->name.c_str());
        //            ImNodes::EndOutputAttribute();
        //        }

        //        ImNodes::EndNode();
        //    }
        //    ImGui::PopStyleColor();

        //    /*    constexpr auto kAddNodePopupId = IM_UNIQUE_ID;
        //        if (ImNodes::IsEditorHovered() && ImGui::IsMouseReleased(ImGuiMouseButton_Right))
        //        {
        //            ImGui::OpenPopup(kAddNodePopupId);
        //        }*/

        //        /*    _nodePopup(kAddNodePopupId, g);
        //            auto changed = _inspectNodes(g, connectedVertices);
        //            _renderLinks(g);
        //            if (m_miniMap.enabled)
        //            {
        //                ImNodes::MiniMap(m_miniMap.size, m_miniMap.location);
        //            }*/

        //    for (const Link& link : links)
        //    {
        //        ImNodes::Link(link.id, link.start_attr, link.end_attr);
        //    }

        //    ImNodes::EndNodeEditor();

        //    {
        //        Link link;
        //        if (ImNodes::IsLinkCreated(&link.start_attr, &link.end_attr))
        //        {
        //            link.id = ++current_id;
        //            links.push_back(link);
        //        }
        //    }

        //    {
        //        int link_id;
        //        if (ImNodes::IsLinkDestroyed(&link_id))
        //        {
        //            auto iter = std::find_if(
        //                links.begin(), links.end(), [link_id](const Link& link) -> bool {
        //                    return link.id == link_id;
        //                });
        //            assert(iter != links.end());
        //            links.erase(iter);
        //        }
        //    }

        //    /*if (ImGui::IsWindowFocused(ImGuiHoveredFlags_ChildWindows))
        //    {
        //        changed |= _handleNewLinks(g, connectedVertices);
        //        changed |= _handleDeletedLinks(g, connectedVertices);
        //        changed |= _handleDeletedNodes(g, connectedVertices);
        //    }*/

        //    ImGui::End();
        //}

        OnDrawUIEx();

        //if (showOverlay)
        //{
        //    DrawOverlay();
        //}

        EndDockSpace();
    }
}