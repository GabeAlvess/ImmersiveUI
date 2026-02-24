#pragma once

#include "VRUIWidget.h"
#include <RE/B/BSModelDB.h>
#include <functional>

namespace vrui
{
    /// Button states for visual feedback
    enum class ButtonState : uint8_t
    {
        Normal,
        Hovered,
        Pressed
    };

    /// A pressable VR button widget.
    /// Supports hover/press visual feedback and callback handlers.
    class VRUIButton : public VRUIWidget
    {
    public:
        using PressCallback = std::function<void(VRUIButton*)>;
        using HoverCallback = std::function<void(VRUIButton*, bool)>;

        /// Create button with procedural quad (label only)
        explicit VRUIButton(const std::string& label,
                            float width = 3.0f, float height = 1.5f);

        /// Create button with custom NIF mesh and optional overhead texture path
        VRUIButton(const std::string& label, const std::string& nifPath, const std::string& texturePath = "",
                   float width = 3.0f, float height = 1.5f);

        void update(float deltaTime) override;

        // --- State ---
        ButtonState getState() const { return _state; }
        /// Get the target scale for the current button state
        float getTargetScale() const { return _targetScale; }
        void setState(ButtonState state);

        // --- Event handlers ---
        void setOnPressHandler(PressCallback callback) { _onPressHandler = std::move(callback); }
        void setOnReleaseHandler(PressCallback callback) { _onReleaseHandler = std::move(callback); }
        void setOnHoverHandler(HoverCallback callback) { _onHoverHandler = std::move(callback); }

        // --- Input dispatch (called by VRMenuManager) ---
        void onRayEnter() override;
        void onRayExit() override;
        void onTriggerPress() override;
        void onTriggerRelease() override;
        
        int getSlotIndex() const { return _slotIndex; }
        void setSlotIndex(int index) { _slotIndex = index; }

        const std::string& getLabel() const { return _label; }
        void setLabel(const std::string& text) { _label = text; refreshLabel(); }
        
        const std::string& getSublabel() const { return _sublabel; }
        void setSublabel(const std::string& text) { _sublabel = text; refreshLabel(); }

        /// Load visual meshes post-construction (vtable is ready)
        void initializeVisuals() override;

    private:

        /// Refreshes the 3D text label using character NIFs
        void refreshLabel();

        std::string _label;
        std::string _sublabel;
        std::string _nifPath;
        std::string _texturePath;
        
        RE::NiPointer<RE::NiNode> _labelNode;
        RE::NiPointer<RE::NiNode> _sublabelNode;

        ButtonState _state = ButtonState::Normal;
        float _targetScale = 1.0f;   // Target scale for smooth lerp
        float _currentScale = 1.0f;  // Current interpolated scale
        int _slotIndex = -1;

        PressCallback _onPressHandler;
        PressCallback _onReleaseHandler;
        HoverCallback _onHoverHandler;
    };
}
