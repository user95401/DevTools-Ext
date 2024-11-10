#pragma once

#include <Geode/ui/GeodeUI.hpp>
class ModSettingsPopup : public FLAlertLayer {};
class ModPopup : public FLAlertLayer {};

#define public_cast(value, member) [](auto* v) { \
	class FriendClass__; \
	using T = std::remove_pointer<decltype(v)>::type; \
	class FriendeeClass__: public T { \
	protected: \
		friend FriendClass__; \
	}; \
	class FriendClass__ { \
	public: \
		auto& get(FriendeeClass__* v) { return v->member; } \
	} c; \
	return c.get(reinterpret_cast<FriendeeClass__*>(v)); \
}(value)

namespace geode::cocos {
    inline std::string frameName(CCNode* node) {
        if (node == nullptr) return "NIL_NODE";
        if (auto textureProtocol = dynamic_cast<CCTextureProtocol*>(node)) {
            if (auto texture = textureProtocol->getTexture()) {
                if (auto spriteNode = dynamic_cast<CCSprite*>(node)) {
                    auto* cachedFrames = CCSpriteFrameCache::sharedSpriteFrameCache()->m_pSpriteFrames;
                    const auto rect = spriteNode->getTextureRect();
                    for (auto [key, frame] : CCDictionaryExt<std::string, CCSpriteFrame*>(cachedFrames)) {
                        if (frame->getTexture() == texture && frame->getRect() == rect) {
                            return key.c_str();
                        }
                    }
                }
                auto* cachedTextures = CCTextureCache::sharedTextureCache()->m_pTextures;
                for (auto [key, obj] : CCDictionaryExt<std::string, CCTexture2D*>(cachedTextures)) {
                    if (obj == texture) {
                        return key.c_str();
                    }
                }
            }
        }
        auto btnSpriteTry = frameName(getChild(node, 0));
        if (
            btnSpriteTry != "NIL_NODE"
            and btnSpriteTry != "CANT_GET_FRAME_NAME"
            ) return btnSpriteTry;
        return "CANT_GET_FRAME_NAME";
    }
    inline std::string getNodeTypeName(CCObject* node) {
#ifdef GEODE_IS_WINDOWS
        return typeid(*node).name();
#else 
        {
            std::string ret;

            int status = 0;
            auto demangle = abi::__cxa_demangle(typeid(*node).name(), 0, 0, &status);
            if (status == 0) {
                ret = demangle;
            }
            free(demangle);

            return ret;
        }
#endif
    }
    inline std::string idOrTypeOfNode(cocos2d::CCNode* node) {
        if (!node) return "NIL NODE";
        auto id = node->getID();
        auto type = getNodeTypeName(node);
        return (id.size() > 1 ? id : type);
    }
    inline std::vector<std::string> getIdsTreeUpToNode(cocos2d::CCNode* start, cocos2d::CCNode* up_to) {
        auto rtn = std::vector<std::string>();
        if (start == nullptr) return rtn;
        //add start
        rtn.insert(rtn.begin(), idOrTypeOfNode(start));
        //add next parents
        auto next_parent = start->getParent();
        while (next_parent != nullptr) {
            rtn.insert(rtn.begin(), idOrTypeOfNode(next_parent));
            next_parent = next_parent->getParent();
            if (up_to == next_parent) next_parent = nullptr;
        }
        //rtn rly
        return rtn;
    }
};

namespace ImGui {
    inline static Ref<CCTextInputNode> g_textInputForSomeFunStuff = CCTextInputNode::create(100.f, 20.f, "asd", "goldFont.fnt");
    inline auto AddTooltip(const char* text) {
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) {
            ImGui::SetTooltip("%s", text);
        }
        return;
    }
    inline auto TryOpenVKForDataInput() {
        //open virtual keyboard for non desktop ones
#ifndef GEODE_IS_DESKTOP
        if (ImGui::IsItemActivated()) {
            g_textInputForSomeFunStuff->onClickTrackNode(true);
        }
        if (ImGui::IsItemDeactivated()) {
            g_textInputForSomeFunStuff->onClickTrackNode(false);
        }
#endif // !GEODE_IS_DESKTOP
    }
    inline auto BetterInputText(const char* label, std::string* str, ImGuiInputTextFlags flags = 0) {
        ImGuiContext& g = *GImGui;
        ImGuiIO& io = g.IO;
        const ImGuiStyle& style = g.Style;
        int lines_count = std::count(str->begin(), str->end(), '\n') + 1;
        const auto size = CalcItemSize({ 0, 0 }, CalcItemWidth(), (g.FontSize * lines_count) + style.FramePadding.y * 2.0f);
        auto textunput_rtn = InputTextMultiline(label, str, size, flags);
        TryOpenVKForDataInput();
        return textunput_rtn;
    }
    inline bool MyTextLink(const char* label) {
        ImGuiWindow* window = GetCurrentWindow();
        if (window->SkipItems)
            return false;

        ImGuiContext& g = *GImGui;
        const ImGuiID id = window->GetID(label);
        const char* label_end = FindRenderedTextEnd(label);

        ImVec2 pos = window->DC.CursorPos;
        ImVec2 size = CalcTextSize(label, label_end, true);
        ImRect bb(pos, ImVec2(pos.x + size.x, pos.y + size.y));
        ItemSize(size, 0.0f);
        if (!ItemAdd(bb, id))
            return false;

        bool hovered, held;
        bool pressed = ButtonBehavior(bb, id, &hovered, &held);
        RenderNavHighlight(bb, id, ImGuiNavHighlightFlags_None);

        const ImU32 col = GetColorU32((held || hovered) ? ImGuiCol_TextDisabled : ImGuiCol_Text);
        RenderNavHighlight(bb, id);

        float line_y = bb.Min.y - ImFloor(g.Font->Descent * g.FontSize * 0.23f);
        window->DrawList->AddLine(ImVec2(bb.Min.x, line_y), ImVec2(bb.Max.x, line_y), col); // FIXME-TEXT: Underline mode.

        PushStyleColor(ImGuiCol_Text, col);
        RenderText(bb.Min, label, label_end);
        PopStyleColor();

        IMGUI_TEST_ENGINE_ITEM_INFO(id, label, g.LastItemData.StatusFlags);
        return pressed;
    }
    inline void ScrollWhenDragging() {
        auto mouse_dt = ImGui::GetIO().MouseDelta;
        ImVec2 delta = ImGui::GetIO().MouseDownDuration[0] > 0.1 ? ImVec2(mouse_dt.x * -1, mouse_dt.y * -1) : ImVec2(0,0);
        ImGuiContext& g = *ImGui::GetCurrentContext();
        ImGuiWindow* window = g.CurrentWindow;
        if (!window) return;
        //bool held = g.IO.MouseDown[0];
        bool hovered = false;
        bool held = false;
        ImGuiID id = window->GetID("##scrolldraggingoverlay");
        ImGui::KeepAliveID(id);
        ImGuiButtonFlags button_flags = ImGuiButtonFlags_MouseButtonLeft;
        if (g.HoveredId == 0) // If nothing hovered so far in the frame (not same as IsAnyItemHovered()!)
            ImGui::ButtonBehavior(window->Rect(), id, &hovered, &held, button_flags);
        if (held && fabs(delta.x) >= 0.1f)
            ImGui::SetScrollX(window, window->Scroll.x + delta.x);
        if (held && fabs(delta.y) >= 0.1f)
            ImGui::SetScrollY(window, window->Scroll.y + delta.y);
    }
}