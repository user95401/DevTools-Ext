
#include "platform/platform.hpp"
#include <Geode/modify/CCKeyboardDispatcher.hpp>
#include <Geode/modify/AchievementNotifier.hpp>
#include <Geode/modify/CCDirector.hpp>
#include <Geode/modify/CCEGLView.hpp>
#include <Geode/modify/CCNode.hpp>
#include "DevTools.hpp"
#include <imgui.h>
#include "ImGui.hpp"

using namespace geode::prelude;

class $modify(CCNode) {
    void sortAllChildren() override {
        if (DevTools::get()->shouldOrderChildren()) {
            CCNode::sortAllChildren();
        }
    }
};

// todo: use shortcuts api once Geode has those
class $modify(CCKeyboardDispatcher) {
    bool dispatchKeyboardMSG(enumKeyCodes key, bool down, bool arr) {
        if (down && (key == KEY_F11 GEODE_MACOS(|| key == KEY_F10))) DevTools::get()->toggle();
        return CCKeyboardDispatcher::dispatchKeyboardMSG(key, down, arr);
    }
};

#include <Geode/modify/CCMenuItem.hpp>
class $modify(CCMenuItem) {
    auto getIdOrSprName() {
        if (not this or this == nullptr) return std::string("bad node");
        auto id = this->getID();
        auto sprite_name = cocos::frameName(this);
        return id.size() > 1 ? id : sprite_name;
    }
    void activate() {
        if (DevTools::get()->getSettings()->openBtnID == this->getIdOrSprName()) {
            DevTools::get()->toggle();
            if (DevTools::get()->getSettings()->openBtnCallOriginal) CCMenuItem::activate();
        }
        else CCMenuItem::activate();
        if (DevTools::get()->isListenForBtnSetup()) {
            DevTools::get()->getSettings()->openBtnID = this->getIdOrSprName();
            DevTools::get()->setListenForBtnSetup(false);
        }
    }
};

#include <Geode/loader/Setting.hpp>
#include <Geode/loader/SettingNode.hpp>
//class ToggleBtnSettingNode : public SettingNode "toggle-btn"
class ToggleBtnSettingValue : public SettingValue {public:ToggleBtnSettingValue(std::string const& key, std::string const& modID, int asd) : SettingValue(key, modID) {}bool load(matjson::Value const& json) override { return true; }; bool save(matjson::Value& json) const override { return true; }; SettingNode* createNode(float width) override;};
class ToggleBtnSettingNode : public SettingNode {
public:
    void ToggleFinallya(CCObject*) {
        DevTools::get()->toggle();
    }
    bool init(ToggleBtnSettingValue* value, float width) {
        if (!SettingNode::init(value)) return false;
        this->setContentSize({ width, 40.f });
        auto item = CCMenuItemSpriteExtra::create(
            CCLabelBMFont::create("Toggle DevTools", "bigFont.fnt"),
            this,
            menu_selector(ToggleBtnSettingNode::ToggleFinallya)
        );
        item->getNormalImage()->setScale(0.5f);
        item->getNormalImage()->setAnchorPoint({ 0.f, 0.5f });
        this->addChild(CCMenu::createWithItem(item));
        item->getParent()->setPositionX(this->getContentHeight() / 2);
        item->getParent()->setPositionY(this->getContentHeight() / 2);
        return true;
    }
    void commit() override { this->dispatchCommitted(); } bool hasUncommittedChanges() override { return false; } bool hasNonDefaultValue() override { return false; } void resetToDefault() override {}
    static ToggleBtnSettingNode* create(ToggleBtnSettingValue* value, float width) {
        auto ret = new ToggleBtnSettingNode;
        if (ret->init(value, width)) {
            ret->autorelease();
            return ret;
        }
        delete ret;
        return nullptr;
    }
}; SettingNode* ToggleBtnSettingValue::createNode(float width) { return ToggleBtnSettingNode::create(this, width); }
$on_mod(Loaded) { Mod::get()->addCustomSetting<ToggleBtnSettingValue>("toggle-btn", 1337); }

class $modify(CCDirector) {
    /*void willSwitchToScene(CCScene * scene) {
        CCDirector::willSwitchToScene(scene);
        DevTools::get()->sceneChanged();
    }*/
    void drawScene() {
        if (!DevTools::get()->shouldUseGDWindow()) {
            return CCDirector::drawScene();
        }
        
        DevTools::get()->setup();

        static GLRenderCtx* gdTexture = nullptr;

        if (!DevTools::get()->shouldPopGame()) {
            if (gdTexture) {
                delete gdTexture;
                gdTexture = nullptr;
            }
            shouldPassEventsToGDButTransformed() = false;
            CCDirector::drawScene();
            return;
        }

        if (shouldUpdateGDRenderBuffer()) {
            if (gdTexture) {
                delete gdTexture;
                gdTexture = nullptr;
            }
            shouldUpdateGDRenderBuffer() = false;
        }

        auto winSize = this->getOpenGLView()->getViewPortRect() * geode::utils::getDisplayFactor();
        if (!gdTexture) {
            gdTexture = new GLRenderCtx({ winSize.size.width, winSize.size.height });
        }

        if (!gdTexture->begin()) {
            delete gdTexture;
            gdTexture = nullptr;
            CCDirector::drawScene();
            DevTools::get()->render(nullptr);
            return;
        }
        CCDirector::drawScene();
        gdTexture->end();

        DevTools::get()->render(gdTexture);

        // if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        //     auto backup_current_context = this->getOpenGLView()->getWindow();
        //     ImGui::UpdatePlatformWindows();
        //     ImGui::RenderPlatformWindowsDefault();
        //     glfwMakeContextCurrent(backup_current_context);
        // }
    }
};

class $modify(CCEGLView) {
    // this is needed for popout mode because we need to render after gd has rendered,
    // but before the buffers have been swapped, which is not possible with just a
    // CCDirector::drawScene hook.
    void swapBuffers() {
        if (!DevTools::get()->shouldUseGDWindow() || !DevTools::get()->shouldPopGame()) {
            DevTools::get()->setup();
            DevTools::get()->render(nullptr);
        }
        CCEGLView::swapBuffers();
    }
};
