#pragma once

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
};

namespace ImGui {
    inline static Ref<CCTextInputNode> g_textInputForSomeFunStuff = CCTextInputNode::create(100.f, 20.f, "asd", "goldFont.fnt");
    inline auto AddTooltip(const char* text) {
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) {
            ImGui::SetTooltip("%s", text);
        }
        return;
    }
    inline auto InputTextMultilineWithAutoHeight(const char* label, std::string* str) {
        ImGuiContext& g = *GImGui;
        ImGuiIO& io = g.IO;
        const ImGuiStyle& style = g.Style;
        int lines_count = std::count(str->begin(), str->end(), '\n') + 1;
        const auto size = CalcItemSize({ 0, 0 }, CalcItemWidth(), (g.FontSize * lines_count) + style.FramePadding.y * 2.0f);
        auto textunput_rtn = InputTextMultiline(label, str, size);
        //open virtual keyboard for non desktop one thing
        if (ImGui::IsItemActivated()) {
            g_textInputForSomeFunStuff->onClickTrackNode(true);
        }
        if (ImGui::IsItemDeactivated()) {
            g_textInputForSomeFunStuff->onClickTrackNode(false);
        }
        return textunput_rtn;
    }
}