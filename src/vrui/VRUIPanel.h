#pragma once

#include "VRUIContainer.h"
#include "VRUIButton.h"

namespace vrui
{
    /// Root-level panel that attaches to a player hand node.
    /// Manages the lifecycle of a complete menu attached to the non-dominant hand.
    class VRUIPanel : public VRUIContainer
    {
    public:
        /// @param name      Panel identifier
        /// @param handNode  Name of the skeleton bone to attach to (e.g. "NPC L Hand [LHnd]")
        explicit VRUIPanel(const std::string& name, float scale = 1.0f);

        /// Attach this panel to a specific NiNode in the player skeleton
        void attachToHandNode(RE::NiNode* handNode, const RE::NiPoint3& offset = {0, 5, 10});

        /// Show/hide the panel (with optional animation)
        virtual void show();
        void hide();
        bool isShown() const { return _shown; }
        
        void setActive(bool active) { _active = active; if (!active) hide(); }
        bool isActive() const { return _active; }

        /// Update panel each frame
        void update(float deltaTime) override;

        /// Collect all interactive buttons in this panel (recursive)
        void collectButtons(std::vector<VRUIButton*>& outButtons);

    private:
        void collectButtonsRecursive(VRUIWidget* widget, std::vector<VRUIButton*>& outButtons);

        bool _shown = false;
        bool _active = true;
        bool _backgroundLoadFailed = false;
        RE::NiNode* _trackingHandNode = nullptr;
        RE::NiPointer<RE::NiNode> _backgroundNode;
        RE::NiPoint3 _offset;
        float _fadeTimer = 0.0f;
        static constexpr float kFadeDuration = 0.2f;
    };
}
