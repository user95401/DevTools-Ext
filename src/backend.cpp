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
    tex2d->initWithData(pixels, kCCTexture2DPixelFormat_RGBA8888, width, height, CCSizeMake(width, height));

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

        //io.MouseSource = ImGuiMouseSource_TouchScreen;
        io.AddMouseSourceEvent(ImGuiMouseSource_TouchScreen);

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

#include <Geode/modify/CCIMEDispatcher.hpp>
class $modify(CCIMEDispatcherExt, CCIMEDispatcher) {
    void dispatchInsertText(const char* text, int len, enumKeyCodes key) {
        //log::debug("{}(\"{}\", {}, {})", __FUNCTION__, text, len, CCKeyboardDispatcher::get()->keyToString(((int)key > 1 ? key : KEY_ApplicationsKey)));
        auto& io = ImGui::GetIO();
        if (!io.WantCaptureKeyboard) CCIMEDispatcher::dispatchInsertText(text, len, key);
        std::string str(text, len);
        io.AddInputCharactersUTF8(str.c_str());
    }
};

#define AddKeyEvent(keyName) if (key == KEY_##keyName) io.AddKeyAnalogEvent(ImGuiKey_##keyName, down, 1.f)
#define AddKeyModEvent(keyName) if (key == KEY_##keyName) io.AddKeyAnalogEvent(ImGuiKey_Mod##keyName, down, 1.f);
#define AddKeyEventArrow(keyName) if (key == KEY_##keyName or key == KEY_Arrow##keyName) io.AddKeyAnalogEvent(ImGuiKey_##keyName##Arrow, down, 1.f)
#include <Geode/modify/CCKeyboardDispatcher.hpp>
class $modify(CCKeyboardDispatcher) {
    bool dispatchKeyboardMSG(enumKeyCodes key, bool down, bool arr) {
        //log::debug("{}({},{},{})", __FUNCTION__, CCKeyboardDispatcher::get()->keyToString(((int)key > 1 ? key : KEY_ApplicationsKey)), down, arr);
        auto& io = ImGui::GetIO(); 
        {
            if (key == KEY_Control) io.AddKeyAnalogEvent(ImGuiKey_ModCtrl, down, 1.f);
            AddKeyModEvent(Shift);
            AddKeyModEvent(Alt);
            //fucking sucks
            AddKeyEventArrow(Left);
            AddKeyEventArrow(Right);
            AddKeyEventArrow(Down);
            AddKeyEventArrow(Up);
            AddKeyEvent(Space);
            AddKeyEvent(Backspace);//0x09,
            AddKeyEvent(Tab);//0x09,
            AddKeyEvent(Enter);//0x0D,
            AddKeyEvent(Pause);//0x13,
            AddKeyEvent(CapsLock);//0x14,
            AddKeyEvent(Escape);//0x1B,
            AddKeyEvent(Space);//0x20,
            AddKeyEvent(PageUp);//0x21,
            AddKeyEvent(PageDown);//0x22,
            AddKeyEvent(End);//0x23,
            AddKeyEvent(Home);//0x24,
            AddKeyEvent(PrintScreen);//0x2C,
            AddKeyEvent(Insert);//0x2D,
            AddKeyEvent(Delete);//0x2E,
            AddKeyEvent(A);//0x41,
            AddKeyEvent(B);//0x42,
            AddKeyEvent(C);//0x43,
            AddKeyEvent(D);//0x44,
            AddKeyEvent(E);//0x45,
            AddKeyEvent(F);//0x46,
            AddKeyEvent(G);//0x47,
            AddKeyEvent(H);//0x48,
            AddKeyEvent(I);//0x49,
            AddKeyEvent(J);//0x4A,
            AddKeyEvent(K);//0x4B,
            AddKeyEvent(L);//0x4C,
            AddKeyEvent(M);//0x4D,
            AddKeyEvent(N);//0x4E,
            AddKeyEvent(O);//0x4F,
            AddKeyEvent(P);//0x50,
            AddKeyEvent(Q);//0x51,
            AddKeyEvent(R);//0x52,
            AddKeyEvent(S);//0x53,
            AddKeyEvent(T);//0x54,
            AddKeyEvent(U);//0x55,
            AddKeyEvent(V);//0x56,
            AddKeyEvent(W);//0x57,
            AddKeyEvent(X);//0x58,
            AddKeyEvent(Y);//0x59,
            AddKeyEvent(Z);//0x5A,
            AddKeyEvent(F1);//0x70,
            AddKeyEvent(F2);//0x71,
            AddKeyEvent(F3);//0x72,
            AddKeyEvent(F4);//0x73,
            AddKeyEvent(F5);//0x74,
            AddKeyEvent(F6);//0x75,
            AddKeyEvent(F7);//0x76,
            AddKeyEvent(F8);//0x77,
            AddKeyEvent(F9);//0x78,
            AddKeyEvent(F10);//0x79,
            AddKeyEvent(F11);//0x7A,
            AddKeyEvent(F12);//0x7B,
            AddKeyEvent(ScrollLock);//0x91,
        };
        if (io.WantCaptureKeyboard) return true;
        return CCKeyboardDispatcher::dispatchKeyboardMSG(key, down, arr);
    }
};
