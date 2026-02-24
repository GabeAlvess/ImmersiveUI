#include "VRUIPanel.h"
#include "VRUIButton.h"
#include "VRUISettings.h"

namespace vrui
{
    VRUIPanel::VRUIPanel(const std::string& name, float scale)
        : VRUIContainer(name, ContainerLayout::VerticalDown, 0.4f, scale), _active(true)
    {
    }

    void VRUIPanel::attachToHandNode(RE::NiNode* handNode, const RE::NiPoint3& offset)
    {
        _offset = offset;
        _trackingHandNode = handNode;

        if (handNode) {
            attachToNode(handNode);
            setLocalPosition(offset);
            logger::info("ImmersiveUI: Panel '{}' attached directly to node '{}'", getName(), handNode->name.c_str());
        } else {
            logger::error("ImmersiveUI: Cannot attach panel '{}' - no target hand node provided", getName());
        }
    }

    void VRUIPanel::show()
    {
        if (!_active) return;
        _fadeTimer = kFadeDuration;
        setVisible(true);

        // Staggered button animation
        std::vector<VRUIButton*> buttons;
        collectButtons(buttons);
        int visibleIdx = 0;
        for (size_t i = 0; i < buttons.size(); ++i) {
            if (buttons[i] && buttons[i]->isVisible()) {
                buttons[i]->startScaleAnimation(visibleIdx * 2); 
                visibleIdx++;
            }
        }

        if (!_shown) {
            _shown = true;
            logger::info("ImmersiveUI: Showing panel '{}'", getName());
        }
    }

    void VRUIPanel::hide()
    {
        if (_shown) {
            _shown = false;
            _fadeTimer = kFadeDuration;
            logger::info("ImmersiveUI: Hiding panel '{}'", getName());
        }
    }

    void VRUIPanel::update(float deltaTime)
    {
        // Handle fade animation
        if (_fadeTimer > 0.0f) {
            _fadeTimer -= deltaTime;
            if (_fadeTimer <= 0.0f) {
                _fadeTimer = 0.0f;
                if (!_shown) {
                    setVisible(false);
                }
            }
        }

        // Apply transforms from settings if correctly attached
        // We rely on the engine's hierarchy since we are attached to the hand node.
        if (_node && _trackingHandNode) {
            auto& settings = VRUISettings::get();
            
            // Basic local transform
            _node->local.translate = _offset;
            
            RE::NiMatrix3 userRot;
            userRot.SetEulerAnglesXYZ(
                settings.menuRotX * (kDegToRad),
                settings.menuRotY * (kDegToRad),
                settings.menuRotZ * (kDegToRad)
            );
            _node->local.rotate = userRot;
            _node->local.scale = settings.menuScale;

            // --- Update Background ---
            if (settings.showBackground) {
                if (!_backgroundNode && !_backgroundLoadFailed) {
                    _backgroundNode = VRUIWidget::loadModelFromNif(settings.backgroundNifPath);
                    if (_backgroundNode) {
                        _node->AttachChild(_backgroundNode.get());
                        _backgroundLoadFailed = false;
                    } else {
                        _backgroundLoadFailed = true;
                    }
                }

                if (_backgroundNode) {
                    _backgroundNode->local.translate = { settings.backgroundOffsetX, settings.backgroundOffsetY, settings.backgroundOffsetZ };
                    _backgroundNode->local.scale = settings.backgroundScale;
                    _backgroundNode->local.rotate.SetEulerAnglesXYZ(
                        settings.backgroundRotX * (kDegToRad),
                        settings.backgroundRotY * (kDegToRad),
                        settings.backgroundRotZ * (kDegToRad)
                    );
                }
            } else if (_backgroundNode) {
                _node->DetachChild(_backgroundNode.get());
                _backgroundNode = nullptr;
            }

            RE::NiUpdateData updateData;
            _node->Update(updateData);
        }

        if (_shown) {
            VRUIContainer::update(deltaTime);
        }
    }

    void VRUIPanel::collectButtons(std::vector<VRUIButton*>& outButtons)
    {
        collectButtonsRecursive(this, outButtons);
    }

    void VRUIPanel::collectButtonsRecursive(VRUIWidget* widget, std::vector<VRUIButton*>& outButtons)
    {
        if (!widget) return;
        
        auto* button = dynamic_cast<VRUIButton*>(widget);
        if (button) {
            outButtons.push_back(button);
        }

        for (auto& child : widget->getChildren()) {
            collectButtonsRecursive(child.get(), outButtons);
        }
    }
}
