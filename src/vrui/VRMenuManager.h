#pragma once

#include "VRUIPanel.h"
#include "VRUISettings.h"

#include <vector>
#include <memory>
#include <filesystem>

namespace vrui
{

    /// Central singleton that orchestrates the ImmersiveUI framework.
    /// Manages panels, processes input, performs raycast, and dispatches events.
    class VRMenuManager
    {
    public:
        static VRMenuManager& get();

        /// Initialize the manager (call once after game data is loaded)
        void initialize();

        /// Called every frame to update all managed panels and input
        void onFrameUpdate(float deltaTime);

        /// Register a panel to be managed
        void registerPanel(std::shared_ptr<VRUIPanel> panel);

        /// Unregister and destroy a panel
        void unregisterPanel(const std::shared_ptr<VRUIPanel>& panel);

        /// Switch active panel without closing menu
        void switchToPanel(const std::string& panelName);

        /// Trigger a full layout refresh on all panels (updates scale, spacing, etc)
        void refreshActivePanels();

        /// Toggle menu visibility (called by activation gesture)
        void toggleMenu();

        /// Check if any menu is currently visible
        bool isMenuOpen() const { return _menuOpen; }

        /// Get the currently hovered widget (if any)
        VRUIWidget* getHoveredWidget() const { return _hoveredWidget; }

        // Page management is delegated to VRUIContainer directly

        // --- External Input Callbacks ---
        // Call these from SkyrimVRTools button listener or KeyHandler

        /// Notify that the grip button state changed on the menu hand
        void onGripButtonChanged(bool pressed);

        /// Notify that the trigger button state changed on the dominant hand
        void onTriggerButtonChanged(bool pressed);

        // --- Laser Access ---
        RE::NiPoint3 getLaserOrigin() const;
        RE::NiPoint3 getLaserDirection() const;

    private:
        VRMenuManager() = default;

        // --- Input processing ---
        void processActivationInput(float deltaTime);
        void processTouchInput(float deltaTime);
        void processTriggerInput();

        // --- Hand node discovery ---
        RE::NiNode* getMenuHandNode() const;
        RE::NiNode* getDominantHandNode() const;
        RE::NiNode* getPlayerSkeletonRoot() const;

        // --- Laser Pointer ---
        void updateLaserPointer(RE::NiNode* dominantHand, float targetDistance);
        void hideLaserPointer();

        // --- Haptic feedback ---
        void triggerHaptic(bool isDominantHand, float intensity, float duration);

        // --- State ---
        // VRIK compatibility flag
        bool _isVRIKInstalled = false;

        bool _initialized = false;
        bool _menuOpen = false;
        float _gripHoldTimer = 0.0f;
        bool _gripWasHeld = false;       // Prevents re-trigger while holding
        bool _triggerPressed = false;

        bool _wasJournalMenuOpen = false;
        std::filesystem::file_time_type _lastIniModifiedTime;

        // External input state (set by callbacks)
        bool _gripButtonDown = false;
        bool _triggerButtonDown = false;

        // --- Current interaction ---
        VRUIWidget* _hoveredWidget = nullptr;
        float _hoverLockTimer = 0.0f;           // Prevents rapid hover/unhover cycling
        static constexpr float kHoverLockTime = 0.16f; // 160ms minimum hover duration

        // Laser pointer mesh (dynamically scaled IconPlane.nif)
        RE::NiPointer<RE::NiNode> _laserPointer;
        bool _laserActive = false;

        // Smoothing state
        RE::NiPoint3 _laserLastPos;
        RE::NiPoint3 _laserLastDir;
        float _laserLastDist = 0.0f;

        // --- Components ---
        std::vector<std::shared_ptr<VRUIPanel>> _panels;
    };
}
