#include <cocos2d.h>
#include "platform/platform.hpp"
#include "DevTools.hpp"
#include "ImGui.hpp"
#include <array>

using namespace cocos2d;

// based off https://github.com/matcool/gd-imgui-cocos

void DevTools::setupPlatform() {
    auto context = ImGui::CreateContext();

    auto& io = ImGui::GetIO();

    io.BackendPlatformUserData = this;
    io.BackendPlatformName = "cocos2d-2.2.3 GD";
    // this is a lie hehe
    io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;

    unsigned char* pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

    auto* tex2d = new CCTexture2D;
    tex2d->initWithData(pixels, kCCTexture2DPixelFormat_RGBA8888, width, height, CCSize(width, height));

    // TODO: not leak this :-)
    tex2d->retain();

    io.Fonts->SetTexID(reinterpret_cast<ImTextureID>(static_cast<intptr_t>(tex2d->getName())));

    auto IniFilePath = (Mod::get()->getSaveDir() / "ImGuiSave.ini");
    static auto IniFilePathStr = std::string(IniFilePath.string());
    io.IniFilename = IniFilePathStr.c_str();

    ImGui::LoadIniSettingsFromDisk(io.IniFilename);
}

void DevTools::newFrame() {
    auto& io = ImGui::GetIO();

    // opengl2 new frame
    auto* director = CCDirector::sharedDirector();
    const auto winSize = director->getWinSize();
    const auto frameSize = director->getOpenGLView()->getFrameSize() * geode::utils::getDisplayFactor();

    // glfw new frame
    io.DisplaySize = ImVec2(frameSize.width, frameSize.height);
    io.DisplayFramebufferScale = ImVec2(
        winSize.width / frameSize.width,
        winSize.height / frameSize.height
    );
    if (director->getDeltaTime() > 0.f) {
        io.DeltaTime = director->getDeltaTime();
    }
    else {
        io.DeltaTime = 1.f / 60.f;
    }

#ifdef GEODE_IS_DESKTOP
    const auto mousePos = toVec2(geode::cocos::getMousePos());
    io.AddMousePosEvent(mousePos.x, mousePos.y);
#endif

    // TODO: text input

    auto* kb = director->getKeyboardDispatcher();
    io.KeyAlt = kb->getAltKeyPressed() || kb->getCommandKeyPressed(); // look
    io.KeyCtrl = kb->getControlKeyPressed();
    io.KeyShift = kb->getShiftKeyPressed();
#ifdef GEODE_IS_WINDOWS
    io.KeyAlt = (GetKeyState(VK_LMENU) & 0x8000) || (GetKeyState(VK_RMENU) & 0x8000); // look
    io.KeyCtrl = (GetKeyState(VK_CONTROL) & 0x8000);
    io.KeyShift = (GetKeyState(VK_SHIFT) & 0x8000);
#endif // GEODE_IS_WINDOWS

}

void DevTools::render(GLRenderCtx* ctx) {
    ccGLBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    this->newFrame();

    ImGui::NewFrame();

    DevTools::get()->draw(ctx);

    ImGui::Render();

    this->renderDrawData(ImGui::GetDrawData());
}

bool DevTools::hasExtension(const std::string& ext) const {
    auto exts = reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS));
    if (exts == nullptr) {
        return false;
    }

    std::string extsStr(exts);
    return extsStr.find(ext) != std::string::npos;
}

namespace {
    static void drawTriangle(const std::array<CCPoint, 3>& poli, const std::array<ccColor4F, 3>& colors, const std::array<CCPoint, 3>& uvs) {
        auto* shader = CCShaderCache::sharedShaderCache()->programForKey(kCCShader_PositionTextureColor);
        shader->use();
        shader->setUniformsForBuiltins();

        ccGLEnableVertexAttribs(kCCVertexAttribFlag_PosColorTex);

        static_assert(sizeof(CCPoint) == sizeof(ccVertex2F), "so the cocos devs were right then");
        
        glVertexAttribPointer(kCCVertexAttrib_Position, 2, GL_FLOAT, GL_FALSE, 0, poli.data());
        glVertexAttribPointer(kCCVertexAttrib_Color, 4, GL_FLOAT, GL_FALSE, 0, colors.data());
        glVertexAttribPointer(kCCVertexAttrib_TexCoords, 2, GL_FLOAT, GL_FALSE, 0, uvs.data());

        glDrawArrays(GL_TRIANGLE_FAN, 0, 3);
    }
}

void DevTools::renderDrawDataFallback(ImDrawData* draw_data) {
    glEnable(GL_SCISSOR_TEST);

    const auto clip_scale = draw_data->FramebufferScale;

    for (int i = 0; i < draw_data->CmdListsCount; ++i) {
        auto* list = draw_data->CmdLists[i];
        auto* idxBuffer = list->IdxBuffer.Data;
        auto* vtxBuffer = list->VtxBuffer.Data;
        for (auto& cmd : list->CmdBuffer) {
            ccGLBindTexture2D(static_cast<GLuint>(reinterpret_cast<intptr_t>(cmd.GetTexID())));

            const auto rect = cmd.ClipRect;
            const auto orig = toCocos(ImVec2(rect.x, rect.y));
            const auto end = toCocos(ImVec2(rect.z, rect.w));
            if (end.x <= orig.x || end.y >= orig.y)
                continue;
            CCDirector::sharedDirector()->getOpenGLView()->setScissorInPoints(orig.x, end.y, end.x - orig.x, orig.y - end.y);

            for (unsigned int i = 0; i < cmd.ElemCount; i += 3) {
                const auto a = vtxBuffer[idxBuffer[cmd.IdxOffset + i + 0]];
                const auto b = vtxBuffer[idxBuffer[cmd.IdxOffset + i + 1]];
                const auto c = vtxBuffer[idxBuffer[cmd.IdxOffset + i + 2]];
                std::array<CCPoint, 3> points = {
                    toCocos(a.pos),
                    toCocos(b.pos),
                    toCocos(c.pos),
                };
                static constexpr auto ccc4FromImColor = [](const ImColor color) {
                    // beautiful
                    return ccc4f(color.Value.x, color.Value.y, color.Value.z, color.Value.w);
                };
                std::array<ccColor4F, 3> colors = {
                    ccc4FromImColor(a.col),
                    ccc4FromImColor(b.col),
                    ccc4FromImColor(c.col),
                };

                std::array<CCPoint, 3> uvs = {
                    ccp(a.uv.x, a.uv.y),
                    ccp(b.uv.x, b.uv.y),
                    ccp(c.uv.x, c.uv.y),
                };

                drawTriangle(points, colors, uvs);
            }
        }
    }

    glDisable(GL_SCISSOR_TEST);
}

void DevTools::renderDrawData(ImDrawData* draw_data) {
    static bool hasVaos = this->hasExtension("GL_ARB_vertex_array_object");
    if (!hasVaos) {
        return this->renderDrawDataFallback(draw_data);
    }

    glEnable(GL_SCISSOR_TEST);

    GLuint vao = 0;
    GLuint vbos[2] = {0, 0};

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glGenBuffers(2, &vbos[0]);

    glBindBuffer(GL_ARRAY_BUFFER, vbos[0]);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbos[1]);

    glEnableVertexAttribArray(kCCVertexAttrib_Position);
    glVertexAttribPointer(kCCVertexAttrib_Position, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (GLvoid*)offsetof(ImDrawVert, pos));

    glEnableVertexAttribArray(kCCVertexAttrib_TexCoords);
    glVertexAttribPointer(kCCVertexAttrib_TexCoords, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (GLvoid*)offsetof(ImDrawVert, uv));

    glEnableVertexAttribArray(kCCVertexAttrib_Color);
    glVertexAttribPointer(kCCVertexAttrib_Color, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(ImDrawVert), (GLvoid*)offsetof(ImDrawVert, col));

    auto* shader = CCShaderCache::sharedShaderCache()->programForKey(kCCShader_PositionTextureColor);
    shader->use();
    shader->setUniformsForBuiltins();

    for (int i = 0; i < draw_data->CmdListsCount; ++i) {
        auto* list = draw_data->CmdLists[i];

        // convert vertex coords to cocos space
        for(size_t j = 0; j < list->VtxBuffer.size(); j++) {
            auto point = toCocos(list->VtxBuffer[j].pos);
            list->VtxBuffer[j].pos = ImVec2(point.x, point.y);
        }

        glBufferData(GL_ARRAY_BUFFER, list->VtxBuffer.Size * sizeof(ImDrawVert), list->VtxBuffer.Data, GL_STREAM_DRAW);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, list->IdxBuffer.Size * sizeof(ImDrawIdx), list->IdxBuffer.Data, GL_STREAM_DRAW);

        for (auto& cmd : list->CmdBuffer) {
            ccGLBindTexture2D(static_cast<GLuint>(reinterpret_cast<intptr_t>(cmd.GetTexID())));

            const auto rect = cmd.ClipRect;
            const auto orig = toCocos(ImVec2(rect.x, rect.y));
            const auto end = toCocos(ImVec2(rect.z, rect.w));
            if (end.x <= orig.x || end.y >= orig.y)
                continue;
            CCDirector::sharedDirector()->getOpenGLView()->setScissorInPoints(orig.x, end.y, end.x - orig.x, orig.y - end.y);

            glDrawElements(GL_TRIANGLES, cmd.ElemCount, GL_UNSIGNED_SHORT, (GLvoid*)(cmd.IdxOffset * sizeof(ImDrawIdx)));
        }
    }

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    glDeleteBuffers(2, &vbos[0]);
    glDeleteVertexArrays(1, &vao);

    glDisable(GL_SCISSOR_TEST);
}

static float SCROLL_SENSITIVITY = 10;

#include <Geode/modify/CCMouseDispatcher.hpp>
class $modify(CCMouseDispatcher) {
    bool dispatchScrollMSG(float y, float x) {
        if(!DevTools::get()->isSetup()) return true;

        auto& io = ImGui::GetIO();
        io.AddMouseWheelEvent(x / SCROLL_SENSITIVITY, -y / SCROLL_SENSITIVITY);

        if (!io.WantCaptureMouse || shouldPassEventsToGDButTransformed()) {
            return CCMouseDispatcher::dispatchScrollMSG(y, x);
        }

        return true;
    }
};

#include <Geode/modify/CCTouchDispatcher.hpp>
class $modify(CCTouchDispatcher) {
    void touches(CCSet* touches, CCEvent* event, unsigned int type) {
        auto& io = ImGui::GetIO();
        auto* touch = static_cast<CCTouch*>(touches->anyObject());

        // for some reason mac can filter out out of touches i think?
        if (touch == nullptr) {
            // i am very lazy to find ccset count
            // i don't even know if the std set in gnustl and libc++ are the same struct
            return;
        }

        const auto pos = toVec2(touch->getLocation());
        io.AddMousePosEvent(pos.x, pos.y);
        if (io.WantCaptureMouse) {
            bool didGDSwallow = false;

            if (DevTools::get()->shouldUseGDWindow() && shouldPassEventsToGDButTransformed()) {
                auto win = ImGui::GetMainViewport()->Size;
                const auto gdRect = getGDWindowRect();
                if (gdRect.Contains(pos) && !DevTools::get()->pausedGame()) {
                    auto relativePos = ImVec2(
                        pos.x - gdRect.Min.x,
                        pos.y - gdRect.Min.y
                    );
                    auto x = (relativePos.x / gdRect.GetWidth()) * win.x;
                    auto y = (1.f - relativePos.y / gdRect.GetHeight()) * win.y;

                    auto pos = toCocos(ImVec2(x, y));
                    touch->setTouchInfo(touch->getID(), pos.x, pos.y);
                    CCTouchDispatcher::touches(touches, event, type);

                    ImGui::SetWindowFocus("Geometry Dash");
                    didGDSwallow = true;
                    io.AddMouseButtonEvent(0, false);
                }
            }

            // TODO: dragging out of gd makes it click in imgui
            if (!didGDSwallow) {
                if (type == CCTOUCHBEGAN || type == CCTOUCHMOVED) {
                    io.AddMouseButtonEvent(0, true);
                }
                else {
                    io.AddMouseButtonEvent(0, false);
                }
            }
        }
        else {
            if (type != CCTOUCHMOVED) {
                io.AddMouseButtonEvent(0, false);
            }
            if (!DevTools::get()->shouldUseGDWindow() || !DevTools::get()->shouldPopGame()) {
                CCTouchDispatcher::touches(touches, event, type);
            }
        }
    }
};

#include <Geode/modify/CCLayer.hpp>
#ifdef GEODE_IS_WINDOWS
    #define VK_TO_ADD_KEY_EVENT(keyName1, keyName2) io.AddKeyAnalogEvent(ImGuiKey_##keyName2, (GetKeyState(VK_##keyName1) & 0x8000), 0.f);
#endif // GEODE_IS_WINDOWS
#define KeyDownToAddKeyEvent(keyName) if (key == KEY_##keyName) { io.AddKeyEvent(ImGuiKey_##keyName, 1); io.AddKeyEvent(ImGuiKey_##keyName, 0); }
#define KeyDownToAddKeyEventArrow(keyName) if (key == KEY_##keyName or key == KEY_Arrow##keyName) { io.AddKeyEvent(ImGuiKey_##keyName##Arrow, 1); io.AddKeyEvent(ImGuiKey_##keyName##Arrow, 0); }
class $modify(CCLayerExt, CCLayer) {
#ifdef GEODE_IS_WINDOWS
    void listenForWinVK(float) {
        auto& io = ImGui::GetIO();
        VK_TO_ADD_KEY_EVENT(LEFT, LeftArrow);
        VK_TO_ADD_KEY_EVENT(RIGHT, RightArrow);
        VK_TO_ADD_KEY_EVENT(DOWN, DownArrow);
        VK_TO_ADD_KEY_EVENT(UP, UpArrow);
        //IS_SENT_BY_DISPATCHER...KEYDOWNTOADDKEYEVENT(SPACE);
        VK_TO_ADD_KEY_EVENT(BACK, Backspace);//0X08,
        VK_TO_ADD_KEY_EVENT(TAB, Tab);//0X09,
        VK_TO_ADD_KEY_EVENT(RETURN, Enter);//0X0D,
        VK_TO_ADD_KEY_EVENT(PAUSE, Pause);//0X13,
        VK_TO_ADD_KEY_EVENT(CAPITAL, CapsLock);//0X14,
        VK_TO_ADD_KEY_EVENT(ESCAPE, Escape);//0X1B,
        VK_TO_ADD_KEY_EVENT(SPACE, Space);//0X20,
        VK_TO_ADD_KEY_EVENT(END, End);//0X23,
        VK_TO_ADD_KEY_EVENT(HOME, Home);//0X24,
        VK_TO_ADD_KEY_EVENT(SNAPSHOT, PrintScreen);//0X2C,
        VK_TO_ADD_KEY_EVENT(INSERT, Insert);//0X2D,
        VK_TO_ADD_KEY_EVENT(DELETE, Delete);//0X2E,
        VK_TO_ADD_KEY_EVENT(LSHIFT, LeftShift);//0XA0,
        VK_TO_ADD_KEY_EVENT(RSHIFT, RightShift);//0XA1
        VK_TO_ADD_KEY_EVENT(LCONTROL, LeftCtrl);//0XA1
        VK_TO_ADD_KEY_EVENT(RCONTROL, RightCtrl);//0XA1
        VK_TO_ADD_KEY_EVENT(CONTROL, ModCtrl);//0XA1
        VK_TO_ADD_KEY_EVENT(CAPITAL, ModAlt);//0XA1
        VK_TO_ADD_KEY_EVENT(F2, F2);
        VK_TO_ADD_KEY_EVENT(F3, F3);
        VK_TO_ADD_KEY_EVENT(F4, F4);
        VK_TO_ADD_KEY_EVENT(F5, F5);
        VK_TO_ADD_KEY_EVENT(F6, F6);
        VK_TO_ADD_KEY_EVENT(F7, F7);
        VK_TO_ADD_KEY_EVENT(F8, F8);
        VK_TO_ADD_KEY_EVENT(F9, F9);
        VK_TO_ADD_KEY_EVENT(F10, F10);
        VK_TO_ADD_KEY_EVENT(F11, F11);
        VK_TO_ADD_KEY_EVENT(F12, F12);
    }
    void keyDown(enumKeyCodes key) {
        auto& io = ImGui::GetIO();
        if (!io.WantCaptureKeyboard) CCLayer::keyDown(key);
        KeyDownToAddKeyEvent(A);//0x41,
        KeyDownToAddKeyEvent(B);//0x42,
        KeyDownToAddKeyEvent(C);//0x43,
        KeyDownToAddKeyEvent(D);//0x44,
        KeyDownToAddKeyEvent(E);//0x45,
        KeyDownToAddKeyEvent(F);//0x46,
        KeyDownToAddKeyEvent(G);//0x47,
        KeyDownToAddKeyEvent(H);//0x48,
        KeyDownToAddKeyEvent(I);//0x49,
        KeyDownToAddKeyEvent(J);//0x4A,
        KeyDownToAddKeyEvent(K);//0x4B,
        KeyDownToAddKeyEvent(L);//0x4C,
        KeyDownToAddKeyEvent(M);//0x4D,
        KeyDownToAddKeyEvent(N);//0x4E,
        KeyDownToAddKeyEvent(O);//0x4F,
        KeyDownToAddKeyEvent(P);//0x50,
        KeyDownToAddKeyEvent(Q);//0x51,
        KeyDownToAddKeyEvent(R);//0x52,
        KeyDownToAddKeyEvent(S);//0x53,
        KeyDownToAddKeyEvent(T);//0x54,
        KeyDownToAddKeyEvent(U);//0x55,
        KeyDownToAddKeyEvent(V);//0x56,
        KeyDownToAddKeyEvent(W);//0x57,
        KeyDownToAddKeyEvent(X);//0x58,
        KeyDownToAddKeyEvent(Y);//0x59,
        KeyDownToAddKeyEvent(Z);//0x5A,
    }
#else //NOT GEODE_IS_WINDOWS
    void keyDown(enumKeyCodes key) {
        auto& io = ImGui::GetIO();
        if (!io.WantCaptureKeyboard) CCLayer::keyDown(key);
        //log::debug("{}(key {}), io.WantCaptureKeyboard: {}", __FUNCTION__, (int)key != -1 ? CCKeyboardDispatcher::get()->keyToString(key) : utils::numToString(key), io.WantCaptureKeyboard);
        //fucking sucks
        KeyDownToAddKeyEventArrow(Left);
        KeyDownToAddKeyEventArrow(Right);
        KeyDownToAddKeyEventArrow(Down);
        KeyDownToAddKeyEventArrow(Up);
        //IS_SENT_BY_DISPATCHER...KeyDownToAddKeyEvent(Space);
        KeyDownToAddKeyEvent(Backspace);//0x08,
        KeyDownToAddKeyEvent(Tab);//0x09,
        KeyDownToAddKeyEvent(Enter);//0x0D,
        KeyDownToAddKeyEvent(Pause);//0x13,
        KeyDownToAddKeyEvent(CapsLock);//0x14,
        KeyDownToAddKeyEvent(Escape);//0x1B,
        KeyDownToAddKeyEvent(Space);//0x20,
        KeyDownToAddKeyEvent(PageUp);//0x21,
        KeyDownToAddKeyEvent(PageDown);//0x22,
        KeyDownToAddKeyEvent(End);//0x23,
        KeyDownToAddKeyEvent(Home);//0x24,
        KeyDownToAddKeyEvent(PrintScreen);//0x2C,
        KeyDownToAddKeyEvent(Insert);//0x2D,
        KeyDownToAddKeyEvent(Delete);//0x2E,
        KeyDownToAddKeyEvent(A);//0x41,
        KeyDownToAddKeyEvent(B);//0x42,
        KeyDownToAddKeyEvent(C);//0x43,
        KeyDownToAddKeyEvent(D);//0x44,
        KeyDownToAddKeyEvent(E);//0x45,
        KeyDownToAddKeyEvent(F);//0x46,
        KeyDownToAddKeyEvent(G);//0x47,
        KeyDownToAddKeyEvent(H);//0x48,
        KeyDownToAddKeyEvent(I);//0x49,
        KeyDownToAddKeyEvent(J);//0x4A,
        KeyDownToAddKeyEvent(K);//0x4B,
        KeyDownToAddKeyEvent(L);//0x4C,
        KeyDownToAddKeyEvent(M);//0x4D,
        KeyDownToAddKeyEvent(N);//0x4E,
        KeyDownToAddKeyEvent(O);//0x4F,
        KeyDownToAddKeyEvent(P);//0x50,
        KeyDownToAddKeyEvent(Q);//0x51,
        KeyDownToAddKeyEvent(R);//0x52,
        KeyDownToAddKeyEvent(S);//0x53,
        KeyDownToAddKeyEvent(T);//0x54,
        KeyDownToAddKeyEvent(U);//0x55,
        KeyDownToAddKeyEvent(V);//0x56,
        KeyDownToAddKeyEvent(W);//0x57,
        KeyDownToAddKeyEvent(X);//0x58,
        KeyDownToAddKeyEvent(Y);//0x59,
        KeyDownToAddKeyEvent(Z);//0x5A,
        KeyDownToAddKeyEvent(F1);//0x70,
        KeyDownToAddKeyEvent(F2);//0x71,
        KeyDownToAddKeyEvent(F3);//0x72,
        KeyDownToAddKeyEvent(F4);//0x73,
        KeyDownToAddKeyEvent(F5);//0x74,
        KeyDownToAddKeyEvent(F6);//0x75,
        KeyDownToAddKeyEvent(F7);//0x76,
        KeyDownToAddKeyEvent(F8);//0x77,
        KeyDownToAddKeyEvent(F9);//0x78,
        KeyDownToAddKeyEvent(F10);//0x79,
        KeyDownToAddKeyEvent(F11);//0x7A,
        KeyDownToAddKeyEvent(F12);//0x7B,
        KeyDownToAddKeyEvent(ScrollLock);//0x91,
        //IS_SET_IN_NEWFRAME...KeyDownToAddKeyEvent(LeftShift);//0xA0,
        //IS_SET_IN_NEWFRAME...KeyDownToAddKeyEvent(RightShift);//0xA1
    }
#endif // GEODE_IS_WINDOWS
    bool init() {
#ifdef GEODE_IS_WINDOWS
        this->schedule(schedule_selector(CCLayerExt::listenForWinVK), 0.0f);
#endif // GEODE_IS_WINDOWS
        return CCLayer::init();
    }
};

#include <Geode/modify/CCIMEDispatcher.hpp>
class $modify(CCIMEDispatcherExt, CCIMEDispatcher) {
    void dispatchInsertText(const char* text, int len, enumKeyCodes key) {
        auto& io = ImGui::GetIO();
        if (!io.WantCaptureKeyboard) CCIMEDispatcher::dispatchInsertText(text, len, key);
        std::string str(text, len);
        io.AddInputCharactersUTF8(str.c_str());
    }
};
