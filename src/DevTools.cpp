
#include <imgui_internal.h>
#include "DevTools.hpp"
#include "fonts/FeatherIcons.hpp"
#include "fonts/OpenSans.hpp"
#include "fonts/GeodeIcons.hpp"
#include "fonts/RobotoMono.hpp"
#include "fonts/SourceCodeProLight.hpp"
#include "platform/platform.hpp"
#include <Geode/loader/Log.hpp>
#include <Geode/loader/Mod.hpp>
#include "ImGui.hpp"

template<>
struct matjson::Serialize<Settings> {
    static Settings from_json(const matjson::Value& value) {
        Settings defaultSettings;
        return Settings {
            .GDInWindow = value.try_get<bool>("game_in_window").value_or(defaultSettings.GDInWindow),
            .attributesInTree = value.try_get<bool>("attributes_in_tree").value_or(defaultSettings.attributesInTree),
            .alwaysHighlight = value.try_get<bool>("always_highlight").value_or(defaultSettings.alwaysHighlight),
            .highlightLayouts = value.try_get<bool>("highlight_layouts").value_or(defaultSettings.highlightLayouts),
            .arrowExpand = value.try_get<bool>("arrow_expand").value_or(defaultSettings.arrowExpand),
            .doubleClickExpand = value.try_get<bool>("double_click_expand").value_or(defaultSettings.doubleClickExpand),
            .orderChildren = value.try_get<bool>("order_children").value_or(defaultSettings.orderChildren),
            .advancedSettings = value.try_get<bool>("advanced_settings").value_or(defaultSettings.advancedSettings),
            .showMemoryViewer = value.try_get<bool>("show_memory_viewer").value_or(defaultSettings.showMemoryViewer),
            .theme = value.try_get<std::string>("theme").value_or(defaultSettings.theme),
            .FontGlobalScale = (float)value.try_get<double>("font_global_scale").value_or(defaultSettings.FontGlobalScale),
            .DearImGuiWindows = value.try_get<bool>("dear_imgui_windows").value_or(defaultSettings.DearImGuiWindows),
            .lang = value.try_get<int>("lang").value_or(defaultSettings.lang),
            .openBtnID = value.try_get<std::string>("openBtnID").value_or(defaultSettings.openBtnID),
            .openBtnCallOriginal = value.try_get<bool>("openBtnCallOriginal").value_or(defaultSettings.openBtnCallOriginal),
        };
    }

    static matjson::Value to_json(const Settings& settings) {
        auto obj = matjson::Object();
        obj["game_in_window"] = settings.GDInWindow;
        obj["attributes_in_tree"] = settings.attributesInTree;
        obj["always_highlight"] = settings.alwaysHighlight;
        obj["highlight_layouts"] = settings.highlightLayouts;
        obj["arrow_expand"] = settings.arrowExpand;
        obj["double_click_expand"] = settings.doubleClickExpand;
        obj["order_children"] = settings.orderChildren;
        obj["advanced_settings"] = settings.advancedSettings;
        obj["show_memory_viewer"] = settings.showMemoryViewer;
        obj["theme"] = settings.theme;
        obj["font_global_scale"] = settings.FontGlobalScale;
        obj["dear_imgui_windows"] = settings.DearImGuiWindows;
        obj["lang"] = settings.lang;
        obj["openBtnID"] = settings.openBtnID;
        obj["openBtnCallOriginal"] = settings.openBtnCallOriginal;
        return obj;
    }

    static bool is_json(matjson::Value const& val) {
        return val.is_object();
    }
};

$on_mod(DataSaved) { DevTools::get()->saveSettings(); }

DevTools* DevTools::get() {
    static auto inst = new DevTools();
    return inst;
}

void DevTools::loadSettings() { m_settings = Mod::get()->getSavedValue<Settings>("settings"); }
void DevTools::saveSettings() { Mod::get()->setSavedValue("settings", m_settings); }

bool DevTools::shouldPopGame() const {
    return m_visible && m_settings.GDInWindow;
}

bool DevTools::pausedGame() const {
    return m_pauseGame;
}

bool DevTools::isSetup() const {
    return m_setup;
}

bool DevTools::shouldOrderChildren() const {
    return m_settings.orderChildren;
}

CCNode* DevTools::getSelectedNode() const {
    return m_selectedNode;
}

void DevTools::selectNode(CCNode* node) {
    m_selectedNode = node;
}

void DevTools::highlightNode(CCNode* node, HighlightMode mode) {
    m_toHighlight.push_back({ node, mode });
}

void DevTools::drawPage(const char* name, void(DevTools::*pageFun)()) {
    if (ImGui::Begin(name, nullptr, ImGuiWindowFlags_HorizontalScrollbar)) {
        (this->*pageFun)();
    }
    ImGui::End();
}

void DevTools::drawPages() {
    const auto size = CCDirector::sharedDirector()->getOpenGLView()->getFrameSize();

    if ((!Mod::get()->setSavedValue("layout-loaded", true) || m_shouldRelayout)) {
        m_shouldRelayout = false;

        auto id = m_dockspaceID;
        ImGui::DockBuilderRemoveNode(id);
        ImGui::DockBuilderAddNode(id, ImGuiDockNodeFlags_PassthruCentralNode);

        auto leftDock = ImGui::DockBuilderSplitNode(m_dockspaceID, ImGuiDir_Left, 0.3f, nullptr, &id);

        auto topLeftDock = ImGui::DockBuilderSplitNode(leftDock, ImGuiDir_Up, 0.4f, nullptr, &leftDock);

        auto bottomLeftTopHalfDock = ImGui::DockBuilderSplitNode(leftDock, ImGuiDir_Up, 0.5f, nullptr, &leftDock);

        auto bottomRightDock = ImGui::DockBuilderSplitNode(id, ImGuiDir_Down, 0.2f, nullptr, &id);

        ImGui::DockBuilderDockWindow("###devtools/tree", topLeftDock);
        ImGui::DockBuilderDockWindow("###devtools/settings", topLeftDock);
        ImGui::DockBuilderDockWindow("###devtools/advanced/settings", topLeftDock);
        ImGui::DockBuilderDockWindow("###devtools/attributes", bottomLeftTopHalfDock);
        ImGui::DockBuilderDockWindow("###devtools/preview", leftDock);
        ImGui::DockBuilderDockWindow("###devtools/geometry-dash", id);
        ImGui::DockBuilderDockWindow("Dear ImGui Style Editor"_LOCALE, id);
        ImGui::DockBuilderDockWindow("Dear ImGui Metrics/Debugger"_LOCALE, id);
        ImGui::DockBuilderDockWindow("Memory viewer"_LOCALE, bottomRightDock);
        ImGui::DockBuilderDockWindow("###devtools/advanced/mod-graph", topLeftDock);
        ImGui::DockBuilderDockWindow("###devtools/advanced/mod-index", topLeftDock);

        ImGui::DockBuilderFinish(id);
    }

    this->drawPage(
        LOCALE_WITH_FEATHER_ICON(FEATHER_GIT_MERGE, " Tree###devtools/tree"),
        &DevTools::drawTree
    );

    this->drawPage(
        LOCALE_WITH_FEATHER_ICON(FEATHER_SETTINGS, " Settings###devtools/settings"),
        &DevTools::drawSettings
    );

    if (m_settings.advancedSettings) {
        this->drawPage(
            LOCALE_WITH_FEATHER_ICON(FEATHER_SETTINGS, " Advanced Settings###devtools/advanced/settings"),
                &DevTools::drawAdvancedSettings
        );
    }

    this->drawPage(
        LOCALE_WITH_FEATHER_ICON(FEATHER_TOOL, " Attributes###devtools/attributes"),
        &DevTools::drawAttributes
    );

    // TODO: fix preview tab
#if 0
    this->drawPage(
        LOCALE_WITH_FEATHER_ICON(FEATHER_DATABASE, " Preview###devtools/preview"),
        &DevTools::drawPreview
    );
#endif

    if (m_showModGraph) {
        this->drawPage(
            LOCALE_WITH_FEATHER_ICON(FEATHER_SHARE_2, " Mod Graph###devtools/advanced/mod-graph"),
            &DevTools::drawModGraph
        );
    }

    if (m_settings.showMemoryViewer) {
        this->drawPage("Memory viewer"_LOCALE, &DevTools::drawMemory);
    }

    if (m_settings.DearImGuiWindows)
    {
        ImGui::Begin("Dear ImGui Style Editor"_LOCALE, &m_settings.DearImGuiWindows); {
            ImGui::ShowStyleEditor();
        } ImGui::End();
        ImGui::ShowMetricsWindow();
    }
}

void DevTools::draw(GLRenderCtx* ctx) {
    if (m_visible) {

        ImGui::SaveIniSettingsToDisk(ImGui::GetIO().IniFilename);

        ImGui::GetIO().FontGlobalScale = m_settings.FontGlobalScale;

        if (m_reloadTheme) {
            applyTheme(m_settings.theme);
            m_reloadTheme = false;
        }

        m_dockspaceID = ImGui::DockSpaceOverViewport(
            nullptr, ImGuiDockNodeFlags_PassthruCentralNode
        );

        this->drawPages();
        if (m_selectedNode) {
            this->highlightNode(m_selectedNode, HighlightMode::Selected);
        }
        if (this->shouldUseGDWindow()) this->drawGD(ctx);
    }
}

void DevTools::setupFonts() {
    auto& io = ImGui::GetIO();

    static const ImWchar def_ranges[] = {
        0x0020, 0x00FF, // Basic Latin + Latin Supplement
        0x0400, 0x052F, // Cyrillic + Cyrillic Supplement
        0x2DE0, 0x2DFF, // Cyrillic Extended-A
        0xA640, 0xA69F, // Cyrillic Extended-B
        0,
    };
    static const ImWchar icon_ranges[] = { FEATHER_MIN_FA, FEATHER_MAX_FA, 0};
    static const ImWchar box_ranges[] = { BOX_DRAWING_MIN_FA, BOX_DRAWING_MAX_FA, 0 };

    static constexpr auto add_font = [](
        void* font, size_t realSize, float size, const ImWchar* range, std::string name = "UndefFont"
        ) {
            auto& io = ImGui::GetIO();
            //tar font
            ImFontConfig config;
            for (int i = 0; i < name.size(); i++) config.Name[i] = name.at(i);
            //tar add
            auto* result = io.Fonts->AddFontFromMemoryTTF(
                font, realSize, size, &config, range
            );
            //add FeatherIcons
            ImFontConfig FeatherIconsConfig;
            FeatherIconsConfig.MergeMode = true;
            auto FeatherIconsNameStr = std::string("FeatherIcons");
            for (int i = 0; i < FeatherIconsNameStr.size(); i++) FeatherIconsConfig.Name[i] = FeatherIconsNameStr.at(i);
            io.Fonts->AddFontFromMemoryTTF(
                Font_FeatherIcons, sizeof(Font_FeatherIcons), size - 4.f, &FeatherIconsConfig, icon_ranges
            );
            //build up all
            io.Fonts->Build();
            return result;
        };
    static constexpr auto add_font_ttf = [](
        std::string name, float size, const ImWchar* range
        ) {
            auto& io = ImGui::GetIO();
            //tar font
            ImFontConfig config;
            for (int i = 0; i < name.size(); i++) config.Name[i] = name.at(i);
            std::string fontPath = CCFileUtils::sharedFileUtils()->fullPathForFilename(name.c_str(), 0);
            auto* result = io.Fonts->AddFontFromFileTTF(
                fontPath.c_str(), size, nullptr, range
            );
            //add FeatherIcons
            ImFontConfig FeatherIconsConfig;
            FeatherIconsConfig.MergeMode = true;
            auto FeatherIconsNameStr = std::string("FeatherIcons");
            for (int i = 0; i < FeatherIconsNameStr.size(); i++) FeatherIconsConfig.Name[i] = FeatherIconsNameStr.at(i);
            io.Fonts->AddFontFromMemoryTTF(
                Font_FeatherIcons, sizeof(Font_FeatherIcons), size - 4.f, &FeatherIconsConfig, icon_ranges
            );
            //build up all
            io.Fonts->Build();
            return result;
        };

    m_defaultFont = add_font(Font_OpenSans, sizeof(Font_OpenSans), 18.f, def_ranges, "OpenSans (defaultFont) [18]");
    m_smallFont = add_font(Font_OpenSans, sizeof(Font_OpenSans), 10.f, def_ranges, "OpenSans (smallFont) [10]");
    m_bigFont = add_font(Font_OpenSans, sizeof(Font_OpenSans), 22.f, def_ranges, "OpenSans (bigFont) [22]");
    m_monoFont = add_font(Font_RobotoMono, sizeof(Font_RobotoMono), 18.f, def_ranges, "RobotoMono (monoFont) [18]");
    m_boxFont = add_font(Font_SourceCodeProLight, sizeof(Font_SourceCodeProLight), 23.f, box_ranges, "SourceCodeProLight (boxFont) [23]");

    ImGui::GetIO().FontDefault = m_defaultFont;
}

void DevTools::setup() {
    if (m_setup) return;
    m_setup = true;

    IMGUI_CHECKVERSION();

    auto ctx = ImGui::CreateContext();

    auto& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    // if this is true then it just doesnt work :( why
    io.ConfigDockingWithShift = false;
    // io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    io.ConfigWindowsResizeFromEdges = true;

    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_NavNoCaptureKeyboard;
    io.NavActive = true;
    io.NavVisible = true;

    this->setupFonts();
    this->setupPlatform();

    setLang(m_settings.lang);

#ifdef GEODE_IS_MOBILE
    ImGui::GetStyle().ScrollbarSize = 42.f;
    //ImGui::GetStyle().TabBarBorderSize = 60.f;
    ImGui::GetStyle().GrabMinSize = 30.f;
    ImGui::GetStyle().ItemSpacing = { 16.f, 16.f };
    ImGui::GetStyle().FramePadding = { 12.f, 10.f };
    //io.ConfigFlags |= ImGuiConfigFlags_IsTouchScreen;
#endif
}

void DevTools::destroy() {
    if (!m_setup) return;
    m_setup = false;
    m_visible = false;

    // crashes :(
    // ImGui::DestroyContext();
}

void DevTools::show(bool visible) {
    m_visible = visible;
}

void DevTools::toggle() {
    this->show(!m_visible);
    if (!m_visible) {
        ImGui::GetIO().WantCaptureMouse = false;
        ImGui::GetIO().WantCaptureKeyboard = false;
    }
}

void DevTools::sceneChanged() {
    m_selectedNode = nullptr;
}

bool DevTools::shouldUseGDWindow() const {
    return Mod::get()->getSettingValue<bool>("should-use-gd-window");
}