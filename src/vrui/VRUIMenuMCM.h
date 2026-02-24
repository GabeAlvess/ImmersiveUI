#pragma once

#include "VRUIPanel.h"
#include "VRUIContainer.h"
#include "VRUIButton.h"

namespace vrui
{
    /**
     * @brief A specialized panel for in-game configuration (Virtual MCM).
     * 
     * Allows adjusting VRUISettings values in real-time, performing saving, 
     * and immediate layout updates.
     */
    class VRUIMenuMCM : public VRUIPanel
    {
    public:
        explicit VRUIMenuMCM(const std::string& name = "VirtualMCM");
        
        /// Initialize the MCM layout and controls
        void initializeVisuals() override;

        /// Override show to recenter content (needs scene graph to be valid)
        void show() override;

        /// Override layout to always re-center content vertically after layout
        void recalculateLayout() override;

        void setOnBackHandler(std::function<void()> handler) { _onBackHandler = handler; }

    private:
        /// Helper: center the MCM container vertically
        void centerContainer();

        /// Helper to create a setting row (Label + Decr Button + Value Display + Incr Button)
        void addSettingRow(const std::string& label, 
                         const std::string& settingKey,
                         float step,
                         std::function<float()> getter,
                         std::function<void(float)> setter);

        std::shared_ptr<VRUIContainer> _container;
        std::function<void()> _onBackHandler;
    };
}
