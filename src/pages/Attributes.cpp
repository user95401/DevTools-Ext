#include "../fonts/FeatherIcons.hpp"
#include <Geode/utils/cocos.hpp>
#include "../DevTools.hpp"
#include <misc/cpp/imgui_stdlib.h>
#include <Geode/binding/CCMenuItemSpriteExtra.hpp>
#include "../platform/utils.hpp"

using namespace geode::prelude;

#define AXIS_GET(Name_) \
    &AxisLayoutOptions::get##Name_, \
    &AxisLayoutOptions::set##Name_

template <class T, class R>
bool checkbox(const char* text, T* ptr, bool(T::* get)(), R(T::* set)(bool)) {
    bool value = (ptr->*get)();
    bool checkbox_rtn = ImGui::Checkbox(text, &value);
    if (checkbox_rtn) {
        (ptr->*set)(value);
        return true;
    }
    return false;
}

template <class T, class R>
bool checkbox(const char* text, T* ptr, bool(T::* get)() const, R(T::* set)(bool)) {
    bool value = (ptr->*get)();
    bool checkbox_rtn = ImGui::Checkbox(text, &value);
    if (checkbox_rtn) {
        (ptr->*set)(value);
        return true;
    }
    return false;
}

static bool andCleanup;
void DevTools::drawNodeAttributes(CCNode* node) {
    if (not node->isRunning()) {
        if (not ImGui::CollapsingHeader("Not running node"_LOCALE)) return;
    }

    //Deselect
    if (ImGui::Button("Deselect"_LOCALE)) return this->selectNode(nullptr);

    ImGui::Text("Remove...");
    ImGui::SameLine();
    if (ImGui::MyTextLink("All Children,"_LOCALE)) return node->removeAllChildrenWithCleanup(andCleanup);
    ImGui::SameLine();
    if (ImGui::MyTextLink("From Parent"_LOCALE)) return node->removeFromParentAndCleanup(andCleanup);
    ImGui::SameLine();
    ImGui::Checkbox("And Cleanup", &andCleanup);

    //Grab
    if (!m_grabbedNode) {
        if (ImGui::Button("Grab This Node"_LOCALE)) this->grabNode(node);
    }
    else {
        if (ImGui::Button("Drop Current Node"_LOCALE)) m_grabbedNode = nullptr;

        if (ImGui::Button("Move out Current Node"_LOCALE)) {
            m_grabbedNode->removeFromParentAndCleanup(0);
            node->addChild(m_grabbedNode);
        }
        ImGui::SameLine();
        if (ImGui::Button("Add Current Node"_LOCALE)) {
            node->addChild(m_grabbedNode);
        }
    }

    ImGui::NewLine();

    //Address
    {
        ImGui::Text("Address: %s"_LOCALE, fmt::to_string(fmt::ptr(node)).c_str());
        ImGui::SameLine();
        if (ImGui::SmallButton(LOCALE_WITH_FEATHER_ICON(FEATHER_COPY, " Copy"))) {
            clipboard::write(
                utils::intToHex(reinterpret_cast<uintptr_t>(node))
            );
        }
        if (node->getUserData()) {
            ImGui::Text("User data: 0x%p"_LOCALE, node->getUserData());
        }
    };

    //nodeID
    if (!node->getID().empty()) {
        std::string nodeID = node->getID();
        ImGui::Text("Node ID: %s"_LOCALE, nodeID.c_str());
        ImGui::SameLine();
        if (ImGui::SmallButton(LOCALE_WITH_FEATHER_ICON(FEATHER_COPY," Copy##copynodeid"))) {
            clipboard::write(nodeID);
        }
    } 
    else {
        ImGui::Text("%s", "Node ID: N/A"_LOCALE);
    }

    //CCMenuItem
    if (auto menuItemNode = typeinfo_cast<CCMenuItem*>(node)) {
        //selector
        const auto selector = menuItemNode->m_pfnSelector;
        if (!selector) {
            std::string addr = "N/A";
            ImGui::Text("CCMenuItem selector: %s"_LOCALE, addr.c_str());
        } 
        else {
            auto addr = addresser::getNonVirtual(selector);
            auto strOffset = formatAddressIntoOffset(addr, true);
            auto strOffsetNoModule = formatAddressIntoOffset(addr, false);
            ImGui::Text("CCMenuItem Selector: %s"_LOCALE, strOffset.c_str());
            ImGui::SameLine();
            if (ImGui::SmallButton(LOCALE_WITH_FEATHER_ICON(FEATHER_COPY, " Copy##copymenuitem"))) {
                clipboard::write(strOffsetNoModule);
            }
        }
        //listener
        const auto listener = menuItemNode->m_pListener;
        auto listenerNode = typeinfo_cast<CCNode*>(listener);
        if (!listener && listenerNode) {
            std::string addr = "N/A";
            ImGui::Text("CCMenuItem Listener: %s"_LOCALE, addr.c_str());
        }
        else {
            auto addr = addresser::getNonVirtual(listener);
            auto strOffset = formatAddressIntoOffset(addr, true);
            auto strOffsetNoModule = formatAddressIntoOffset(addr, false);
            ImGui::Text("CCMenuItem listener: %s"_LOCALE, strOffsetNoModule.c_str());
            ImGui::SameLine();
            if (ImGui::SmallButton("Select Node##selmenuitemlistener"_LOCALE)) {
                m_selectedNode = listenerNode;
            }
            if (ImGui::IsItemHovered()) {
                m_isAboutToSelectNode = listenerNode;
                highlightNode(listenerNode, HighlightMode::Hovered); 
            }
            ImGui::SameLine();
            if (ImGui::SmallButton(LOCALE_WITH_FEATHER_ICON(FEATHER_COPY, " Copy##copymenuitemlistener"))) {
                clipboard::write(strOffsetNoModule);
            }
        }
        //call stuff
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 0.f, 0.f });
        ImGui::Text("menuItemNode->");
        ImGui::SameLine();
        if (ImGui::MyTextLink("activate();")) {
            menuItemNode->activate();
        }
        ImGui::SameLine();
        ImGui::Text(" / ");
        ImGui::SameLine();
        if (ImGui::MyTextLink("selected();")) {
            menuItemNode->selected();
        }
        ImGui::SameLine();
        ImGui::Text(" / ");
        ImGui::SameLine();
        if (ImGui::MyTextLink("unselected();")) {
            menuItemNode->unselected();
        }
        ImGui::PopStyleVar();
    }

    ImGui::NewLine();

    //pos
    {
        float pos[2] = {
            node->getPositionX(),
            node->getPositionY()
        };
        if (ImGui::Button(U8STR(FEATHER_COPY"##copypos"))) {
            clipboard::write(fmt::format(
                "CCPointMake({}{}, {}{})",
                pos[0], ((int)pos[0] == pos[0] ? ".f" : "f"), pos[1], ((int)pos[1] == pos[1] ? ".f" : "f")
            ));
        } ImGui::SameLine();
        ImGui::AddTooltip("\"CCPointMake({}, {})\"<={pos[0], pos[1]}");
        ImGui::DragFloat2("Position"_LOCALE, pos); ImGui::TryOpenVKForDataInput();
        ImGui::AddTooltip("Gets the position (x,y) of the node in OpenGL coordinates."_LOCALE);
        node->setPosition(pos[0], pos[1]);
    };

    //scale
    {
        float scale[3] = { node->getScale(), node->getScaleX(), node->getScaleY() };
        if (ImGui::Button(U8STR(FEATHER_COPY"##copyscale"))) clipboard::write(fmt::format("({}, {})", scale[1], scale[2])); ImGui::SameLine();
        ImGui::AddTooltip("\"({}, {})\"<={scale[1], scale[2]}");
        ImGui::DragFloat3("Scale"_LOCALE, scale, 0.025f); ImGui::TryOpenVKForDataInput();
        ImGui::AddTooltip("The scale factor of the node."_LOCALE);
        if (node->getScale() != scale[0]) {
            node->setScale(scale[0]);
        }
        else {
            node->setScale(scale[1], scale[2]);
        }
    };

    //rot
    {
        float rot[3] = { node->getRotation(), node->getRotationX(), node->getRotationY() };
        if (ImGui::Button(U8STR(FEATHER_COPY"##copyrot"))) clipboard::write(rot[1] != 0.f and rot[2] != 0.f ? fmt::format("{} {} {}", rot[0], rot[1], rot[2]) : fmt::to_string(rot[0])); ImGui::SameLine();
        ImGui::AddTooltip("\"{}\"<={rot[0]} or \"{} {} {}\"<={rot[0], rot[1], rot[2]} if rot[1,2] != 0");
        ImGui::DragFloat3("Rotation"_LOCALE, rot); ImGui::TryOpenVKForDataInput();
        ImGui::AddTooltip("The rotation of the node in degrees."_LOCALE);
        if (node->getRotation() != rot[0]) {
            node->setRotation(rot[0]);
        }
        else {
            node->setRotationX(rot[1]);
            node->setRotationY(rot[2]);
        }
    };

    //skew
    {
        float skew[2] = { node->getSkewX(), node->getSkewY() };
        if (ImGui::Button(U8STR(FEATHER_COPY"##copyskew"))) clipboard::write(fmt::format("{} {}", skew[0], skew[1])); ImGui::SameLine();
        ImGui::AddTooltip("\"{} {}\"<={skew[0], skew[1]}");
        ImGui::DragFloat2("Skew"_LOCALE, skew); ImGui::TryOpenVKForDataInput();
        ImGui::AddTooltip("The skew angle of the node in degrees."_LOCALE);
        node->setSkewX(skew[0]);
        node->setSkewY(skew[1]);
    };

    //anchor
    {
        auto anchor = node->getAnchorPoint();
        if (ImGui::Button(U8STR(FEATHER_COPY"##copyanchor"))) clipboard::write(fmt::format("CCPointMake({}, {})", anchor.x, anchor.y)); ImGui::SameLine();
        ImGui::AddTooltip("\"CCPointMake({}, {})\"<={anchor.x, anchor.y}");
        ImGui::DragFloat2("Anchor Point"_LOCALE, &anchor.x, 0.05f, 0.f, 1.f);ImGui::TryOpenVKForDataInput();
        ImGui::AddTooltip("The anchor point in percent."_LOCALE);
        node->setAnchorPoint(anchor);
    };

    //contentSize
    {
        auto contentSize = node->getContentSize();
        if (ImGui::Button(U8STR(FEATHER_COPY"##copycontentsize"))) clipboard::write(fmt::format("CCSizeMake({}, {})", contentSize.width, contentSize.height)); ImGui::SameLine();
        ImGui::AddTooltip("\"CCSizeMake({}, {})\"<={contentSize.width, contentSize.height}");
        ImGui::DragFloat2("Content Size"_LOCALE, &contentSize.width); ImGui::TryOpenVKForDataInput();
        ImGui::AddTooltip("The untransformed size of the node."_LOCALE);
        if (contentSize != node->getContentSize()) {
            node->setContentSize(contentSize);
            node->updateLayout();
        }
    };

    //zOrder
    {
        int zOrder = node->getZOrder();
        if (ImGui::Button(U8STR(FEATHER_COPY"##copyzorder"))) clipboard::write(fmt::format("{}", zOrder)); ImGui::SameLine();
        ImGui::AddTooltip("\"{}\"<={zOrder}");
        ImGui::InputInt("Z Order"_LOCALE, &zOrder); ImGui::TryOpenVKForDataInput();
        ImGui::AddTooltip("The z order which stands for the drawing order."_LOCALE);
        if (node->getZOrder() != zOrder) node->setZOrder(zOrder);
    }

    //checkboxens
    {
        checkbox(
            "Ignore Anchor Point for Position"_LOCALE,
            node,
            &CCNode::isIgnoreAnchorPointForPosition,
            &CCNode::ignoreAnchorPointForPosition);
        ImGui::AddTooltip("Whether the anchor point will be (0,0) when you position this node."_LOCALE);

        checkbox("Visible"_LOCALE, node, &CCNode::isVisible, &CCNode::setVisible);
        ImGui::AddTooltip("Whether the node is visible."_LOCALE);

        if (auto spriteNode = typeinfo_cast<CCSprite*>(node)) {
#ifndef GEODE_IS_ARM_MAC //Undefined symbols...?
            checkbox("Flip X"_LOCALE, spriteNode, &cocos2d::CCSprite::isFlipX, &cocos2d::CCSprite::setFlipX);
            ImGui::AddTooltip(
				"Whether the sprite is flipped horizontally or not." "\n"
                "It only flips the texture of the sprite, and not the texture of the sprite's children."_LOCALE
            );
            checkbox("Flip Y"_LOCALE, spriteNode, &cocos2d::CCSprite::isFlipY, &cocos2d::CCSprite::setFlipY);
            ImGui::AddTooltip(
                "Whether the sprite is flipped vertically or not." "\n"
                "It only flips the texture of the sprite, and not the texture of the sprite's children."_LOCALE
            );
#endif
        }
    };
    
    //layer
    if (auto layer = typeinfo_cast<CCLayer*>(node)) {
        
        checkbox("Touch Enabled"_LOCALE, layer, &CCLayer::isTouchEnabled, &CCLayer::setTouchEnabled);
        ImGui::AddTooltip(
            "You can enable / disable touch events with this property.""\n"
            "Only the touches of this node will be affected.This \"method\" is not propagated to it's children."_LOCALE
        );
        
        checkbox("Accelerometer Enabled"_LOCALE, layer, &CCLayer::isAccelerometerEnabled, &CCLayer::setAccelerometerEnabled);
        ImGui::AddTooltip(
            "whether or not it will receive Accelerometer events."_LOCALE
        );

        checkbox("Keypad Enabled"_LOCALE, layer, &CCLayer::isKeypadEnabled, &CCLayer::setKeypadEnabled);
        ImGui::AddTooltip(
            "whether or not it will receive keypad events"_LOCALE
        );

        checkbox("Keyboard Enabled"_LOCALE, layer, &CCLayer::isKeyboardEnabled, &CCLayer::setKeyboardEnabled);

        checkbox("Mouse Enabled"_LOCALE, layer, &CCLayer::isMouseEnabled, &CCLayer::setMouseEnabled);

    }

    //rgbaNode
    if (auto rgbaNode = typeinfo_cast<CCRGBAProtocol*>(node)) {
        checkbox("Cascade Color"_LOCALE, rgbaNode, &CCRGBAProtocol::isCascadeColorEnabled, &CCRGBAProtocol::setCascadeColorEnabled);
        ImGui::AddTooltip("Whether or not color should be propagated to its children."_LOCALE);
        auto color = rgbaNode->getColor();
        float _color[4] = { color.r / 255.f, color.g / 255.f, color.b / 255.f, rgbaNode->getOpacity() / 255.f };
        if (ImGui::Button(U8STR(FEATHER_COPY"##copycolor"))) clipboard::write(fmt::format("ccc3({}, {}, {})", color.r, color.g, color.b)); ImGui::SameLine();
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("\"ccc3({}, {}, {})\"<={color.r, color.g, color.b}");
        if (ImGui::Button(U8STR(FEATHER_EYE"##copyop"))) clipboard::write(fmt::format("{}", rgbaNode->getOpacity())); ImGui::SameLine();
        ImGui::AddTooltip("\"{}, {}, {}\"<={rgbaNode->getOpacity()}");
        if (ImGui::ColorEdit4("Color"_LOCALE, _color)) {
            rgbaNode->setColor({
                static_cast<GLubyte>(_color[0] * 255),
                static_cast<GLubyte>(_color[1] * 255),
                static_cast<GLubyte>(_color[2] * 255)
            });
            rgbaNode->setOpacity(static_cast<GLubyte>(_color[3] * 255));
        }
    }
    
    //labelNode
    if (auto labelNode = typeinfo_cast<CCLabelProtocol*>(node)) {
        std::string str = labelNode->getString();
        if (ImGui::Button(U8STR(FEATHER_COPY"##copylabeltext"))) clipboard::write(fmt::format("{}", str)); ImGui::SameLine();
        ImGui::AddTooltip("\"{}\"<={str}");
        //ImGui::InputText("Text"_LOCALE, &str, { 0, 20 });
        if (ImGui::BetterInputText("Text"_LOCALE, &str)) {
            labelNode->setString(str.c_str());
        }
    }

#ifndef GEODE_IS_MACOS //Undefined symbols...
    //bitMapLabelNode
    auto bitMapLabelNode = typeinfo_cast<CCLabelBMFont*>(node);
    auto ttfLabelNode = typeinfo_cast<CCLabelTTF*>(node);
    if (bitMapLabelNode or ttfLabelNode) {
        std::string font = bitMapLabelNode ? bitMapLabelNode->getFntFile() : ttfLabelNode->getFontName();
        //copy
        if (ImGui::Button(U8STR(FEATHER_COPY"##copylabelfont"))) clipboard::write(fmt::format("{}", font)); ImGui::SameLine();
        ImGui::AddTooltip("\"{}\"<={str}");
        //imput
        auto InputText = ImGui::InputText("Font"_LOCALE, &font); ImGui::TryOpenVKForDataInput();
        auto fileExists = cocos::fileExistsInSearchPaths(font.c_str());
        //force
        if (not fileExists) {
            ImGui::TextColored(
                (ttfLabelNode != nullptr ? ImVec4(1.f, 0.7f, 0.3f, 1.f) : ImVec4(1.f, 0.4f, 0.4f, 1.f )),
                "\"%s\" %s", font.c_str(), "file don't exists in search paths!"_LOCALE
            );
        };
        //setup
        if (InputText) {
            if (bitMapLabelNode and fileExists) bitMapLabelNode->setFntFile(font.c_str());
            else if (ttfLabelNode) ttfLabelNode->setFontName(font.c_str());
        };
    }
#endif

    //Display Frame
    if (auto spriteNode = typeinfo_cast<CCSprite*>(node)) {
        std::string frameName = cocos::frameName(spriteNode);
        //copy
        if (ImGui::Button(U8STR(FEATHER_COPY"##copysprframename"))) clipboard::write(fmt::format("{}", frameName)); ImGui::SameLine();
        ImGui::AddTooltip("\"{}\"<={frameName}");
        //imput
        auto InputText = ImGui::InputText("Display Frame"_LOCALE, &frameName); ImGui::TryOpenVKForDataInput();
        auto tempFrame = CCSpriteFrameCache::get()->spriteFrameByName(frameName.c_str());
        //force
        if (not tempFrame) {
            ImGui::TextColored(
                ImVec4(1.f, 0.4f, 0.4f, 1.f ),
                "\"%s\" %s", frameName.c_str(), " is not standing for any cached frame! (failed at getting by name)"_LOCALE
            );
        };
        //setup
        if (InputText) {
            if (spriteNode and tempFrame) spriteNode->setDisplayFrame(tempFrame);
        };
    }

    //Texture name
    if (auto textureProtocol = typeinfo_cast<CCTextureProtocol*>(node)) {
        if (auto texture = textureProtocol->getTexture()) {
            auto* cachedTextures = CCTextureCache::sharedTextureCache()->m_pTextures;
            for (auto [key, obj] : CCDictionaryExt<std::string, CCTexture2D*>(cachedTextures)) {
                if (obj == texture) {
                    ImGui::TextWrapped("Texture name: %s"_LOCALE, key.c_str());
                    break;
                }
            }
        }
    }

    //Touch Priority
    if (auto delegate = typeinfo_cast<CCTouchDelegate*>(node)) {
        if (auto handler = CCTouchDispatcher::get()->findHandler(delegate)) {
            auto priority = handler->getPriority();

            if (ImGui::DragInt("Touch Priority"_LOCALE, &priority, .03f)) {
                CCTouchDispatcher::get()->setPriority(priority, handler->getDelegate());
            }ImGui::TryOpenVKForDataInput();
        }
    }

    //next is about layouts goes here

    ImGui::NewLine();
    ImGui::Separator();
    ImGui::NewLine();

    if (ImGui::CollapsingHeader("Layouts"_LOCALE)) {

        if (auto rawOpts = node->getLayoutOptions()) {
            ImGui::Text("Layout options: %s"_LOCALE, typeid(*rawOpts).name());
            if (ImGui::Button(LOCALE_WITH_FEATHER_ICON(FEATHER_REFRESH_CW, " Update Parent Layout"))) {
                if (auto parent = node->getParent()) {
                    parent->updateLayout();
                }
            }
            if (auto opts = typeinfo_cast<AxisLayoutOptions*>(rawOpts)) {
                bool updateLayout = false;

                ImGui::Text("%s", "Auto Scale"_LOCALE);
                auto updateAxis = false;
                int autoScale = opts->getAutoScale() ? opts->getAutoScale().value() + 1 : 0;
                updateAxis |= ImGui::RadioButton("Default"_LOCALE, &autoScale, 0);
                ImGui::SameLine();
                updateAxis |= ImGui::RadioButton("Enable"_LOCALE, &autoScale, 1);
                ImGui::SameLine();
                updateAxis |= ImGui::RadioButton("Disable"_LOCALE, &autoScale, 2);
                if (updateAxis) {
                    switch (autoScale) {
                    case 0: opts->setAutoScale(std::nullopt); break;
                    case 1: opts->setAutoScale(true); break;
                    case 2: opts->setAutoScale(false); break;
                    }
                    updateLayout = true;
                }

                if (checkbox("Break Line"_LOCALE, opts, AXIS_GET(BreakLine))) {
                    updateLayout = true;
                }
                if (checkbox("Same Line"_LOCALE, opts, AXIS_GET(SameLine))) {
                    updateLayout = true;
                }

                auto prio = opts->getScalePriority();
                if (ImGui::DragInt("Scale Priority"_LOCALE, &prio, .03f)) {
                    opts->setScalePriority(prio);
                    updateLayout = true;
                }ImGui::TryOpenVKForDataInput();

                if (updateLayout && node->getParent()) {
                    node->getParent()->updateLayout();
                }
            }
            else if (auto opts = typeinfo_cast<AnchorLayoutOptions*>(rawOpts)) {
                bool updateLayout = false;

                auto offset = opts->getOffset();
                ImGui::DragFloat2("Offset"_LOCALE, &offset.x);
                if (opts->getOffset() != offset) {
                    opts->setOffset(offset);
                    updateLayout = true;
                }ImGui::TryOpenVKForDataInput();

                auto anchor = static_cast<int>(opts->getAnchor());
                auto updateAnchor = false;
                ImGui::BeginTable("anchor-table", 3);
                ImGui::TableNextColumn();
                updateAnchor |= ImGui::RadioButton("Top Left"_LOCALE, &anchor, static_cast<int>(Anchor::TopLeft));
                updateAnchor |= ImGui::RadioButton("Left"_LOCALE, &anchor, static_cast<int>(Anchor::Left));
                updateAnchor |= ImGui::RadioButton("Bottom Left"_LOCALE, &anchor, static_cast<int>(Anchor::BottomLeft));
                ImGui::TableNextColumn();
                updateAnchor |= ImGui::RadioButton("Top"_LOCALE, &anchor, static_cast<int>(Anchor::Top));
                updateAnchor |= ImGui::RadioButton("Center"_LOCALE, &anchor, static_cast<int>(Anchor::Center));
                updateAnchor |= ImGui::RadioButton("Bottom"_LOCALE, &anchor, static_cast<int>(Anchor::Bottom));
                ImGui::TableNextColumn();
                updateAnchor |= ImGui::RadioButton("Top Right"_LOCALE, &anchor, static_cast<int>(Anchor::TopRight));
                updateAnchor |= ImGui::RadioButton("Right"_LOCALE, &anchor, static_cast<int>(Anchor::Right));
                updateAnchor |= ImGui::RadioButton("Bottom Right"_LOCALE, &anchor, static_cast<int>(Anchor::BottomRight));
                ImGui::EndTable();

                if (updateAnchor) {
                    if (opts->getAnchor() != static_cast<Anchor>(anchor)) {
                        opts->setAnchor(static_cast<Anchor>(anchor));
                        updateLayout = true;
                    }
                }

                if (updateLayout && node->getParent()) {
                    node->getParent()->updateLayout();
                }

            }
        }
        else {
            if (ImGui::Button(LOCALE_WITH_FEATHER_ICON(FEATHER_PLUS, " Add AxisLayoutOptions"))) {
                node->setLayoutOptions(AxisLayoutOptions::create());
            }
            if (ImGui::Button(LOCALE_WITH_FEATHER_ICON(FEATHER_PLUS, " Add AnchorLayoutOptions"))) {
                node->setLayoutOptions(AnchorLayoutOptions::create());
            }
        }

        if (auto rawLayout = node->getLayout()) {
            ImGui::Text("Layout: %s"_LOCALE, typeid(*rawLayout).name());

            if (ImGui::Button(LOCALE_WITH_FEATHER_ICON(FEATHER_REFRESH_CW, " Update Layout"))) {
                node->updateLayout();
            }
            ImGui::SameLine();
            if (ImGui::Button(LOCALE_WITH_FEATHER_ICON(FEATHER_PLUS, " Add Test Child"))) {
                auto spr = CCSprite::create("GJ_button_01.png");
                auto btn = CCMenuItemSpriteExtra::create(spr, node, nullptr);
                node->addChild(btn);
                node->updateLayout();
            }
            if (auto layout = typeinfo_cast<AxisLayout*>(rawLayout)) {
                bool updateLayout = false;

                auto axis = static_cast<int>(layout->getAxis());
                ImGui::Text("%s", "Axis"_LOCALE);
                auto updateAxis = false;
                updateAxis |= ImGui::RadioButton("Row"_LOCALE, &axis, static_cast<int>(Axis::Row));
                ImGui::SameLine();
                updateAxis |= ImGui::RadioButton("Column"_LOCALE, &axis, static_cast<int>(Axis::Column));
                if (updateAxis) {
                    if (layout->getAxis() != static_cast<Axis>(axis)) {
                        node->setContentSize({
                            node->getContentSize().height,
                            node->getContentSize().width
                            });
                    }
                    layout->setAxis(static_cast<Axis>(axis));
                    updateLayout = true;
                }

                auto axisReverse = layout->getAxisReverse();
                if (ImGui::Checkbox("Flip Axis Direction"_LOCALE, &axisReverse)) {
                    layout->setAxisReverse(axisReverse);
                    updateLayout = true;
                }
                axisReverse = layout->getCrossAxisReverse();
                if (ImGui::Checkbox("Flip Cross Axis Direction"_LOCALE, &axisReverse)) {
                    layout->setCrossAxisReverse(axisReverse);
                    updateLayout = true;
                }

                {
                    auto align = static_cast<int>(layout->getAxisAlignment());
                    ImGui::Text("%s", "Axis Alignment"_LOCALE);
                    bool updateAlign = false;
                    updateAlign |= ImGui::RadioButton(
                        "Start"_LOCALE, &align, static_cast<int>(AxisAlignment::Start)
                    );
                    ImGui::SameLine();
                    updateAlign |= ImGui::RadioButton(
                        "Center"_LOCALE, &align, static_cast<int>(AxisAlignment::Center)
                    );
                    ImGui::SameLine();
                    updateAlign |= ImGui::RadioButton(
                        "End"_LOCALE, &align, static_cast<int>(AxisAlignment::End)
                    );
                    ImGui::SameLine();
                    updateAlign |= ImGui::RadioButton(
                        "Even"_LOCALE, &align, static_cast<int>(AxisAlignment::Even)
                    );
                    ImGui::SameLine();
                    updateAlign |= ImGui::RadioButton(
                        "Between"_LOCALE, &align, static_cast<int>(AxisAlignment::Between)
                    );
                    if (updateAlign) {
                        layout->setAxisAlignment(static_cast<AxisAlignment>(align));
                        updateLayout = true;
                    }
                }

                {
                    auto align = static_cast<int>(layout->getCrossAxisAlignment());
                    ImGui::Text("%s", "Cross Axis Alignment"_LOCALE);
                    bool updateAlign = false;
                    updateAlign |= ImGui::RadioButton(
                        "Start##cross0"_LOCALE, &align, static_cast<int>(AxisAlignment::Start)
                    );
                    ImGui::SameLine();
                    updateAlign |= ImGui::RadioButton(
                        "Center##cross1"_LOCALE, &align, static_cast<int>(AxisAlignment::Center)
                    );
                    ImGui::SameLine();
                    updateAlign |= ImGui::RadioButton(
                        "End##cross2"_LOCALE, &align, static_cast<int>(AxisAlignment::End)
                    );
                    ImGui::SameLine();
                    updateAlign |= ImGui::RadioButton(
                        "Even##cross3"_LOCALE, &align, static_cast<int>(AxisAlignment::Even)
                    );
                    ImGui::SameLine();
                    updateAlign |= ImGui::RadioButton(
                        "Between##cross4"_LOCALE, &align, static_cast<int>(AxisAlignment::Between)
                    );
                    if (updateAlign) {
                        layout->setCrossAxisAlignment(static_cast<AxisAlignment>(align));
                        updateLayout = true;
                    }
                }

                {
                    auto align = static_cast<int>(layout->getCrossAxisLineAlignment());
                    ImGui::Text("%s", "Cross Axis Line Alignment"_LOCALE);
                    bool updateAlign = false;
                    updateAlign |= ImGui::RadioButton(
                        "Start##crossline0"_LOCALE, &align, static_cast<int>(AxisAlignment::Start)
                    );
                    ImGui::SameLine();
                    updateAlign |= ImGui::RadioButton(
                        "Center##crossline1"_LOCALE, &align, static_cast<int>(AxisAlignment::Center)
                    );
                    ImGui::SameLine();
                    updateAlign |= ImGui::RadioButton(
                        "End##crossline2"_LOCALE, &align, static_cast<int>(AxisAlignment::End)
                    );
                    ImGui::SameLine();
                    updateAlign |= ImGui::RadioButton(
                        "Even##crossline3"_LOCALE, &align, static_cast<int>(AxisAlignment::Even)
                    );
                    ImGui::SameLine();
                    updateAlign |= ImGui::RadioButton(
                        "Between##crossline4"_LOCALE, &align, static_cast<int>(AxisAlignment::Between)
                    );
                    if (updateAlign) {
                        layout->setCrossAxisLineAlignment(static_cast<AxisAlignment>(align));
                        updateLayout = true;
                    }
                }

                auto gap = layout->getGap();
                if (ImGui::DragFloat("Gap"_LOCALE, &gap)) {
                    layout->setGap(gap);
                    updateLayout = true;
                }ImGui::TryOpenVKForDataInput();

                auto autoScale = layout->getAutoScale();
                if (ImGui::Checkbox("Auto Scale"_LOCALE, &autoScale)) {
                    layout->setAutoScale(autoScale);
                    updateLayout = true;
                }

                auto grow = layout->getGrowCrossAxis();
                if (ImGui::Checkbox("Grow Cross Axis"_LOCALE, &grow)) {
                    layout->setGrowCrossAxis(grow);
                    updateLayout = true;
                }

                auto overflow = layout->getCrossAxisOverflow();
                if (ImGui::Checkbox("Allow Cross Axis Overflow"_LOCALE, &overflow)) {
                    layout->setCrossAxisOverflow(overflow);
                    updateLayout = true;
                }

                if (updateLayout) {
                    node->updateLayout();
                }
            }
        }
        else {
            if (ImGui::Button(LOCALE_WITH_FEATHER_ICON(FEATHER_PLUS, " Add AxisLayout"))) {
                node->setLayout(AxisLayout::create());
            }
            if (ImGui::Button(LOCALE_WITH_FEATHER_ICON(FEATHER_PLUS, " Add AnchorLayout"))) {
                node->setLayout(AnchorLayout::create());
            }
        }

    }

    ImGui::NewLine();
    ImGui::Separator();
    ImGui::NewLine();

    if (ImGui::CollapsingHeader("Generated Node Setup Code"_LOCALE)) {
        auto floatStr = [](float val) {
            return fmt::format("{}{}", val, (int)val == val ? ".f" : "f");
            };
        auto pos = node->getPosition();
        float scale[2] = { node->getScaleX(), node->getScaleY() };
        float rot[2] = { node->getRotationX(), node->getRotationY() };
        float skew[2] = { node->getSkewX(), node->getSkewY() };
        auto anchor = node->getAnchorPoint();
        auto contentSize = node->getContentSize();
        auto spriteNode = typeinfo_cast<CCSprite*>(node);
        auto rgbaNode = typeinfo_cast<CCRGBAProtocol*>(node);
        auto color = (rgbaNode ? rgbaNode->getColor() : ccc3(0, 0, 0));
        auto labelNode = typeinfo_cast<CCLabelProtocol*>(node);
        std::string setFontPart = "";
#ifndef GEODE_IS_MACOS //Undefined symbols...
        auto bitMapLabelNode = typeinfo_cast<CCLabelBMFont*>(node);
        auto ttfLabelNode = typeinfo_cast<CCLabelTTF*>(node);
        if (bitMapLabelNode or ttfLabelNode) setFontPart = fmt::format(
            "\n\t" "nodeToSetup->{}(\"{}\");",
            (bitMapLabelNode ? "setFntFile" : "setFontName"),
            (bitMapLabelNode ? bitMapLabelNode->getFntFile() : ttfLabelNode->getFontName())
        );
#endif
        std::string code = { std::string()
            + fmt::format("if (auto& nodeToSetup = YOUR_NODE_VAR)") + " {"
            + "\n\t" + fmt::format("nodeToSetup->setPosition(CCPointMake({}, {}));", floatStr(pos.x), floatStr(pos.y))
            + (
                scale[0] != scale[1]
                ? //one by one
                "\n\t" + fmt::format("nodeToSetup->setScaleX({});", floatStr(scale[0]))
                + "\n\t" + fmt::format("nodeToSetup->setScaleY({});", floatStr(scale[1]))
                : //both
                "\n\t" + fmt::format("nodeToSetup->setScale({});", floatStr(scale[0]))
                )
            + (
                rot[0] != rot[1]
                ? //one by one
                "\n\t" + fmt::format("nodeToSetup->setRotationX({});", floatStr(rot[0]))
                + "\n\t" + fmt::format("nodeToSetup->setRotationY({});", floatStr(rot[1]))
                : //both
                "\n\t" + fmt::format("nodeToSetup->setRotation({});", floatStr(rot[0]))
                )
            + "\n\t" + fmt::format("nodeToSetup->setSkewX({});", floatStr(skew[0]))
            + "\n\t" + fmt::format("nodeToSetup->setSkewY({});", floatStr(skew[1]))
            + "\n\t" + fmt::format("nodeToSetup->setAnchorPoint(CCPointMake({}, {}));", floatStr(anchor.x), floatStr(anchor.y))
            + "\n\t" + fmt::format("nodeToSetup->setContentSize(CCSizeMake({}, {})); //may be calculated set! (remove if u dont touched that)", floatStr(contentSize.width), floatStr(contentSize.height))
            + "\n\t" + fmt::format("nodeToSetup->setZOrder({});", node->getZOrder())
            + "\n\t" + fmt::format("nodeToSetup->ignoreAnchorPointForPosition({});", node->isIgnoreAnchorPointForPosition())
            + "\n\t" + fmt::format("nodeToSetup->setVisible({});", node->isVisible())
#ifndef GEODE_IS_ARM_MAC //Undefined symbols...?
            + (spriteNode ? "\n\t" + fmt::format("nodeToSetup->setFlipX({});", spriteNode->isFlipX()) : "")
            + (spriteNode ? "\n\t" + fmt::format("nodeToSetup->setFlipY({});", spriteNode->isFlipY()) : "")
#endif
            + (rgbaNode ? "\n\t" + fmt::format("nodeToSetup->setCascadeColorEnabled({});", rgbaNode->isCascadeColorEnabled()) : "")
            + (rgbaNode ? "\n\t" + fmt::format("nodeToSetup->setColor(ccc3({}, {}, {}));", color.r, color.g, color.b) : "")
            + (rgbaNode ? "\n\t" + fmt::format("nodeToSetup->setOpacity({});", rgbaNode->getOpacity()) : "")
            + (labelNode ? "\n\t" + fmt::format("nodeToSetup->setString(\"{}\");", labelNode->getString()) : "")
            + setFontPart
            + (spriteNode ? "\n\t" + fmt::format("nodeToSetup->setDisplayFrame(CCSpriteFrameCache::get()->spriteFrameByName(\"{}\"));", cocos::frameName(spriteNode)) : "")
            + "\n}"
        };
        ImGui::SetNextItemWidth(ImGui::GetWindowWidth() - ImGui::GetFontSize() - 50.f);
        ImGui::BetterInputText("##codegen", &code);
        ImGui::SameLine();
        if (ImGui::SmallButton(U8STR(FEATHER_COPY"##copycodegen"))) clipboard::write(code);
    };

    ImGui::NewLine();

}

void DevTools::drawAttributes() {
    if (m_selectedNode.data()) {
        this->drawNodeAttributes(m_selectedNode);
    } 
    else {
        ImGui::TextWrapped("%s", "Select a Node to Edit in the Scene or Tree"_LOCALE);
    }

    ImGui::NewLine();

    auto tarID = getMod()->getSavedValue<std::string>("tarSelNodeID");
    if (ImGui::BetterInputText("##tarID", &tarID)) getMod()->setSavedValue("tarSelNodeID", tarID);

    ImGui::SameLine();

    if (ImGui::Button("Find Node By ID")) {
        auto topNode = m_selectedNode ? m_selectedNode : CCDirector::get()->m_pRunningScene;
        if (topNode) m_selectedNode = topNode->getChildByIDRecursive(tarID);
    }

    auto tarSelNodeTag = getMod()->getSavedValue<int>("tarSelNodeTag");
    if (ImGui::InputInt("##tarSelNodeTag", &tarSelNodeTag)) getMod()->setSavedValue("tarSelNodeTag", tarSelNodeTag);
    ImGui::TryOpenVKForDataInput();

    ImGui::SameLine();

    if (ImGui::Button("Find Node By Tag")) {
        auto topNode = m_selectedNode ? m_selectedNode : CCDirector::get()->m_pRunningScene;
        if (topNode) m_selectedNode = findFirstChildRecursive<CCNode>(
            topNode, [tarSelNodeTag](CCNode* node) {return node->getTag() == tarSelNodeTag; }
        );
    }

}
