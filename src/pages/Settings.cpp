#include "../fonts/FeatherIcons.hpp"
#include "../DevTools.hpp"
#include <Geode/loader/Loader.hpp>
#include <Geode/loader/Mod.hpp>
#include <Geode/utils/ranges.hpp>
// #include <Geode/binding/FMODAudioEngine.hpp>
#include <Geode/modify/AppDelegate.hpp>
#include <fmod.hpp>
#include <numeric>

using namespace geode::prelude;

static float RAINBOW_HUE = 0.f;

void DevTools::drawSettings() {
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 1.f, 1.f });

    //Checkboxes
    {
        // TODO: fix this option as it hasnt worked in a while lol
#if 0
        //GD in Window
        ImGui::Checkbox("GD in Window"_LOCALE, &m_settings.GDInWindow);
        ImGui::AddTooltip("Show GD inside a window when DevTools are open"_LOCALE);
#endif
        //makes tree ugly
#if 0
        //Attributes in Tree
        ImGui::Checkbox("Attributes in Tree"_LOCALE, &m_settings.attributesInTree);
        ImGui::AddTooltip("Show node attributes in the Tree"_LOCALE);
#endif
        //Highlight Nodes
        ImGui::Checkbox("Highlight Nodes"_LOCALE, &m_settings.alwaysHighlight);
        ImGui::AddTooltip(
            "Always highlight nodes when hovered in the Tree.\n"
            "When disabled, you can highlight by pressing Shift."_LOCALE
        );
        //Highlight Layouts
        ImGui::Checkbox("Highlight Layouts"_LOCALE, &m_settings.highlightLayouts);
        ImGui::AddTooltip("Highlights the borders of all layouts applied to nodes"_LOCALE);
        //Arrow to Expand
        ImGui::Checkbox("Arrow to Expand"_LOCALE, &m_settings.arrowExpand);
        ImGui::AddTooltip(
            "If enabled, expanding nodes in the Tree only works with the arrow.\n"
            "Makes selecting nodes less annoying."_LOCALE
        );
        //doubleClickExpand
        ImGui::Checkbox("Double click to Expand"_LOCALE, &m_settings.doubleClickExpand);
        ImGui::AddTooltip("Need double-click to open node in Tree."_LOCALE);
        //Order Node Children
        ImGui::Checkbox("Order Node Children"_LOCALE, &m_settings.orderChildren);
        ImGui::AddTooltip(
            "When enabled (default behavior) node children are sorted by Z Order.\n"
            "When disabled, children have the same order they do during init functions (maybe).\n"
            "As a side effect to disabling this, things may render incorrectly."_LOCALE
        );
        //Advanced Settings
        ImGui::Checkbox("Advanced Settings"_LOCALE, &m_settings.advancedSettings);
        ImGui::AddTooltip("Shows advanced settings. Mostly useful only for development of Geode itself."_LOCALE);
        //Show Memory Viewer
        ImGui::Checkbox("Show Memory Viewer"_LOCALE, &m_settings.showMemoryViewer);
        ImGui::AddTooltip("Shows the memory viewer window."_LOCALE);
        //next ones shoud be latest always
        ImGui::Checkbox("Show ImGui Debug"_LOCALE, &m_settings.DearImGuiWindows);
        ImGui::AddTooltip("Shows ImGui Style Editor and Metrics."_LOCALE);//Shows ImGui Style Editor and Metrics
    };

    ImGui::PopStyleVar();

    ImGui::Separator();

    // TODO: undo later
#if 0
    ImGui::Text("GD Window");
    auto winSize = CCDirector::get()->getWinSize();
    auto frameSize = GameManager::get()->resolutionForKey(GameManager::get()->m_resolution);
    auto fps = roundf(1 / CCDirector::get()->getAnimationInterval());
    auto ratio = std::gcd(static_cast<int>(frameSize.width), static_cast<int>(frameSize.height));

    std::string text = "";
    text += "Custom";
    text.push_back('\0');
    for (int i = 1; i < 28; i++) {
        auto size = GameManager::get()->resolutionForKey(i);
        text += fmt::format("{}x{}", size.width, size.height);
        text.push_back('\0');
    }
    int selectedResolution = GameManager::get()->m_resolution;

    static CCSize customResolution = frameSize;

    if (ImGui::Combo("##devtools/resolution", &selectedResolution, text.c_str())) {
        GameManager::get()->m_resolution = selectedResolution;

        // TODO: idk how to do this on macos
#ifdef GEODE_IS_WINDOWS
        if (selectedResolution != 0) {
            auto size = GameManager::get()->resolutionForKey(selectedResolution);
            CCEGLView::get()->resizeWindow(size.width, size.height);
        }
        else {
            CCEGLView::get()->resizeWindow(customResolution.width, customResolution.height);
        }
        CCEGLView::get()->centerWindow();
#endif
    }

    if (selectedResolution == 0) {
        int size[2] = {
            static_cast<int>(customResolution.width),
            static_cast<int>(customResolution.height),
        };
        if (ImGui::DragInt2("Size", size)) {
            size[0] = std::fabs(size[0]);
            size[1] = std::fabs(size[1]);
            customResolution = CCSizeMake(size[0], size[1]);
        }
#ifdef GEODE_IS_WINDOWS
        if (ImGui::Button("Apply##size-apply")) {
            GameManager::get()->m_resolution = 0;
            CCEGLView::get()->resizeWindow(customResolution.width, customResolution.height);
            CCEGLView::get()->centerWindow();
        }
#endif
    }

    ImGui::TextWrapped(
        "GL Size: %dx%d",
        static_cast<int>(winSize.width),
        static_cast<int>(winSize.height)
    );
    ImGui::TextWrapped(
        "Frame Size: %dx%d",
        static_cast<int>(frameSize.width),
        static_cast<int>(frameSize.height)
    );
    ImGui::TextWrapped("FPS: %d", static_cast<int>(fps));
    ImGui::TextWrapped(
        "Aspect Ratio: %d:%d",
        static_cast<int>(frameSize.width / ratio),
        static_cast<int>(frameSize.height / ratio)
    );

    static Ref<CCSet> PAUSED_TARGETS = nullptr;
    if (ImGui::Button(m_pauseGame ? "Resume Game" : "Pause Game")) {
        m_pauseGame ^= 1;
        if (m_pauseGame) {
            FMODAudioEngine::sharedEngine()->m_globalChannel->setPaused(true);
            PAUSED_TARGETS = CCDirector::get()->getScheduler()->pauseAllTargets();
        }
        else if (PAUSED_TARGETS) {
            FMODAudioEngine::sharedEngine()->m_globalChannel->setPaused(false);
            CCDirector::get()->getScheduler()->resumeTargets(PAUSED_TARGETS);
        }
    }

    ImGui::Separator();
#endif

    //Select Theme
    {
        ImGui::PushFont(m_bigFont);
        ImGui::Text("%s", "Theme"_LOCALE);
        ImGui::PopFont();
        static auto SELECTED = static_cast<int>(getThemeIndex(m_settings.theme));
        if (ImGui::Combo("##devtools/theme", &SELECTED, (ranges::join(getThemeOptions(), std::string(1, '\0')) + '\0').c_str())) {
            m_settings.theme = getThemeAtIndex(SELECTED);
            m_reloadTheme = true;
        }
        ImGui::AddTooltip("Select Theme"_LOCALE);
        ImGui::DragFloat("Font Size"_LOCALE, &m_settings.FontGlobalScale, 0.01f, 1.0f, 3.0f);
    };

    ImGui::Separator();

    //Select Language
    {
        ImGui::PushFont(m_bigFont);
        ImGui::Text("%s", "Language"_LOCALE);
        ImGui::PopFont();
        if (ImGui::Combo("##devtools/lang", &m_settings.lang, (ranges::join(lang_list, std::string(1, '\0')) + '\0').c_str())) {
            setLang(m_settings.lang);
        }
        ImGui::AddTooltip("Select Language"_LOCALE);
        if (std::string(lang_inf).size() > 1) ImGui::TextWrapped("%s", lang_inf);
    }

    ImGui::Separator();

    //Setup Open DevTools Button in Game UI
    {
        ImGui::PushFont(m_bigFont);
        ImGui::TextWrapped("%s", "Setup Open DevTools Button in Game UI"_LOCALE);
        ImGui::PopFont();

        ImGui::TextWrapped("%s\"%s\"", "Current button is "_LOCALE, (m_settings.openBtnID.c_str()));

        ImGui::Checkbox("Call original"_LOCALE, &m_settings.openBtnCallOriginal);

        if (ImGui::Button("Unset Button"_LOCALE)) {
            m_settings.openBtnID = "none (dont use any btn)";
            m_listenForBtnSetup = false;
        }

        ImGui::SameLine();

        if (ImGui::Button(m_listenForBtnSetup ? "Listening for click..."_LOCALE : "Select Button"_LOCALE)) {
            m_listenForBtnSetup = !m_listenForBtnSetup;
        }
    };

    ImGui::Separator();

    //info section idk
    {
        ImGui::PushFont(m_bigFont);
        ImGui::TextWrapped("%s", "DevTools"_LOCALE);
        ImGui::PopFont();

        ImGui::TextWrapped("%s", "Developed by "_LOCALE);

        RAINBOW_HUE += 0.01f;
        if (RAINBOW_HUE >= 1.f) {
            RAINBOW_HUE = 0.f;
        }

        float hue = RAINBOW_HUE;

        ImVec4 color;
        color.w = 1.f;
        for (auto c : std::string("Geode Team"_LOCALE)) {
            hue += 0.04f;
            ImGui::SameLine(0.f, 0.f);
            ImGui::ColorConvertHSVtoRGB(hue, .5f, 1.f, color.x, color.y, color.z);
            ImGui::TextColored(color, "%c", c);
        }

        ImGui::TextWrapped(
            "Running Geode %s, DevTools %s"
            "\nPlatform: %s"
            "\nGame Version: %.3lf"_LOCALE,
            Loader::get()->getVersion().toString().c_str(),
            Mod::get()->getVersion().toString().c_str(),
            GEODE_PLATFORM_NAME,
            GEODE_GD_VERSION
        );

        if (ImGui::Button("Reset Layout"_LOCALE)) {
            m_shouldRelayout = true;
        }
        ImGui::AddTooltip("Reset the windows docking and stuff."_LOCALE);

        if (ImGui::Button("Mod Settings"_LOCALE)) {
            //try find already opened mod popup of devtools
            auto devToolsPopup = findFirstChildRecursive<ModPopup>(
                CCDirector::get()->m_pRunningScene,
                [](auto testNode) {
                    auto firstLabel = findFirstChildRecursive<CCLabelBMFont>(testNode, [](auto) { return true; });
                    if (firstLabel; Mod::get()->getName() == firstLabel->getString()) return true;
                    return false;
                }
            );
            if (not devToolsPopup) openInfoPopup(Mod::get());
        }
        ImGui::AddTooltip("Actually opens mod popup where you can open settings, read smth and go some links for example."_LOCALE);

        if (ImGui::Button("Toggle DevTools"_LOCALE)) {
            DevTools::get()->toggle();
        }
        ImGui::AddTooltip("Yea, close this thing."_LOCALE);
    };
}

class $modify(AppDelegate) {
    void applicationWillEnterForeground() override {
        AppDelegate::applicationWillEnterForeground();
        if (DevTools::get()->pausedGame()) {
            // TODO: undo later
            FMODAudioEngine::sharedEngine()->m_globalChannel->setPaused(true);
        }
    }
};
