
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
        if (DevTools::get()->isListenForKeySetup()) {
            DevTools::get()->getSettings()->toggleKey = key;
            DevTools::get()->setListenForKeySetup(false);
            return 1;
        }
        if (down && (key == DevTools::get()->getSettings()->toggleKey)) DevTools::get()->toggle();
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

//GJ_checkOff_001.png
//GJ_checkOn_001.png
class ToggleImageUpdater : CCNode {
public:
    void customSetup(float) {
        if (auto __this = typeinfo_cast<CCSprite*>(this)) {
            auto visible = DevTools::get()->m_visible;
            auto GJ_checkOn_001 = CCSpriteFrameCache::get()->spriteFrameByName("GJ_checkOn_001.png");
            auto GJ_checkOff_001 = CCSpriteFrameCache::get()->spriteFrameByName("GJ_checkOff_001.png");
            __this->setDisplayFrame(visible ? GJ_checkOn_001 : GJ_checkOff_001);
        }
    }

};
auto modpopupevent = +[](ModPopupUIEvent* event)
    {
        if (event->getModID() == GEODE_MOD_ID) {
            if (auto support = typeinfo_cast<CCMenuItemSpriteExtra*>(event->getPopup()->getChildByIDRecursive("support"))) {
                support->setZOrder(-1);
                if (auto menu = support->getParent()) menu->updateLayout();

                CCMenuItemExt::assignCallback<CCMenuItem>(
                    support, [](auto) {
                        DevTools::get()->toggle();
                    }
                );

                auto image = typeinfo_cast<CCSprite*>(support->getNormalImage());
                image->setOpacity(255);
                image->setColor(ccWHITE);
                ((ToggleImageUpdater*)image)->customSetup(0.0f);
                image->schedule(schedule_selector(ToggleImageUpdater::customSetup), 0.01f);
            }
        }
        // You should always propagate Geode UI events
        return ListenerResult::Propagate;
    };
$execute{ new EventListener<EventFilter<ModPopupUIEvent>>(modpopupevent); }