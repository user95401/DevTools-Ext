#pragma once

#include "platform/platform.hpp"
#include <imgui.h>
#include "themes.hpp"
#include "ImGui.hpp"
#include <cocos2d.h>
#include <Geode/utils/cocos.hpp>
#include <Geode/utils/addresser.hpp>
#include <Geode/loader/Loader.hpp>
#include <Geode/loader/ModMetadata.hpp>
#include <unordered_map>

#include <lang/incl.h>

using namespace geode::prelude;

#include <utils.hpp>

enum class HighlightMode {
    Selected,
    Hovered,
    Layout,
};

struct Settings {
    bool GDInWindow = true;
    bool attributesInTree = false;
    bool alwaysHighlight = true;
    bool highlightLayouts = false;
    bool arrowExpand = false;
    bool orderChildren = true;
    bool advancedSettings = false;
    bool showMemoryViewer = false;
    std::string theme = DARK_THEME;
    float FontGlobalScale = 1.0;
    bool DearImGuiWindows = false;
    int lang = 0;
#ifdef GEODE_IS_DESKTOP
    std::string openBtnID = "none (dont use any btn)";
#else
    std::string openBtnID = "more-games-button";
#endif
    bool openBtnCallOriginal = false;
};

class DevTools {
    bool m_visible = false;
    bool m_setup = false;
    bool m_reloadTheme = true;
    bool m_shouldRelayout = false;
    bool m_showModGraph = false;
    bool m_pauseGame = false;
    bool m_listenForBtnSetup = false;
    Settings m_settings;
    ImGuiID m_dockspaceID;
    ImFont* m_defaultFont  = nullptr;
    ImFont* m_bigFont    = nullptr;
    ImFont* m_smallFont    = nullptr;
    ImFont* m_monoFont     = nullptr;
    ImFont* m_boxFont      = nullptr;
    Ref<CCNode> m_selectedNode;
    std::vector<std::pair<CCNode*, HighlightMode>> m_toHighlight;

    void setupFonts();
    void setupPlatform();

    void drawTree();
    void drawTreeBranch(CCNode* node, size_t index);
    void drawSettings();
    void drawAdvancedSettings();
    void drawNodeAttributes(CCNode* node);
    void drawAttributes();
    void drawPreview();
    void drawNodePreview(CCNode* node);
    void drawHighlight(CCNode* node, HighlightMode mode);
    void drawLayoutHighlights(CCNode* node);
    void drawGD(GLRenderCtx* ctx);
    void drawModGraph();
    void drawModGraphNode(Mod* node);
    ModMetadata inputMetadata(void* treePtr, ModMetadata metadata);
    void drawPage(const char* name, void(DevTools::* fun)());
    void drawPages();
    void drawMemory();
    void draw(GLRenderCtx* ctx);

    void newFrame();
    void renderDrawData(ImDrawData*);
    void renderDrawDataFallback(ImDrawData*);

    bool hasExtension(const std::string& ext) const;

    DevTools() { loadSettings(); }

public:
    static DevTools* get();
    void loadSettings();
    void saveSettings();
    auto getSettings() { return &m_settings; };

    auto isListenForBtnSetup() { return m_listenForBtnSetup; };
    auto setListenForBtnSetup(bool listenForBtnSetup) { m_listenForBtnSetup = listenForBtnSetup; };

    bool shouldUseGDWindow() const;

    bool shouldPopGame() const;
    bool pausedGame() const;
    bool isSetup() const;
    bool shouldOrderChildren() const;

    CCNode* getSelectedNode() const;
    void selectNode(CCNode* node);
    void highlightNode(CCNode* node, HighlightMode mode);

    void sceneChanged();

    void render(GLRenderCtx* ctx);

    // setup ImGui & DevTools
    void setup();
    void destroy();

    void show(bool visible);
    void toggle();
};
