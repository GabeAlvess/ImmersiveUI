#include "VRUIMenuMCM.h"
#include "VRUISettings.h"
#include "VRMenuManager.h"
#include <cstdio>

namespace vrui
{
    VRUIMenuMCM::VRUIMenuMCM(const std::string& name)
        : VRUIPanel(name)
    {
    }

    void VRUIMenuMCM::initializeVisuals()
    {
        VRUIPanel::initializeVisuals();
        
        // Main container with enough vertical spacing for ease of use
        _container = std::make_shared<VRUIContainer>(_name + "_MCMContainer", ContainerLayout::VerticalDown, 1.5f);
        _container->setLocalPosition(RE::NiPoint3{ 0.0f, 0.0f, 0.0f });
        addElement(_container);

        auto& settings = VRUISettings::get();

        // 1. Menu Overall
        addSettingRow("Menu Scale", "fMenuScale", 0.05f,
            [&settings]() { return settings.menuScale; },
            [&settings](float val) { settings.menuScale = val; });

        addSettingRow("Button Spacing", "fButtonSpacing", 0.1f,
            [&settings]() { 
                return settings.buttonSpacing; 
            },
            [&settings, this](float val) { 
                settings.buttonSpacing = val;
                // MCM's own container needs a refresh if we want it to react to ITSELF
                // Though usually we want it to affect the MAIN menu.
            });

        // 2. Position Controls
        addSettingRow("Pos Y (Forward)", "fMenuOffsetY", 0.5f,
            [&settings]() { return settings.menuOffsetY; },
            [&settings](float val) { settings.menuOffsetY = val; });

        addSettingRow("Pos Z (Up)", "fMenuOffsetZ", 0.5f,
            [&settings]() { return settings.menuOffsetZ; },
            [&settings](float val) { settings.menuOffsetZ = val; });

        // 3. Rotation Controls
        addSettingRow("Rot X (Pitch)", "fMenuRotX", 5.0f,
            [&settings]() { return settings.menuRotX; },
            [&settings](float val) { settings.menuRotX = val; });

        addSettingRow("Rot Y (Roll)", "fMenuRotY", 5.0f,
            [&settings]() { return settings.menuRotY; },
            [&settings](float val) { settings.menuRotY = val; });

        addSettingRow("Rot Z (Yaw)", "fMenuRotZ", 5.0f,
            [&settings]() { return settings.menuRotZ; },
            [&settings](float val) { settings.menuRotZ = val; });

        // Padding row (empty spacing)
        _container->addElement(std::make_shared<VRUIWidget>("Padding", 0, 1.0f));

        // 4. Navigation Buttons
        auto btnRow = std::make_shared<VRUIContainer>(_name + "_nav", ContainerLayout::HorizontalCenter, 1.0f);
        
        auto backBtn = std::make_shared<VRUIButton>("Back", "immersiveUI\\slot01.nif", "textures\\test.dds", 3.0f, 1.0f);
        backBtn->setLabel("BACK");
        backBtn->setOnPressHandler([this](VRUIButton*) {
            if (_onBackHandler) _onBackHandler();
        });
        btnRow->addElement(backBtn);

        auto saveBtn = std::make_shared<VRUIButton>("Save", "immersiveUI\\slot01.nif", "textures\\test.dds", 3.0f, 1.0f);
        saveBtn->setLabel("SAVE INI");
        saveBtn->setOnPressHandler([](VRUIButton*) {
            auto& settings = VRUISettings::get();
            settings.save(settings.getDefaultIniPath());
            RE::DebugNotification("ImmersiveUI: Settings Saved to INI!");
        });
        btnRow->addElement(saveBtn);
        
        _container->addElement(btnRow);
    }

    void VRUIMenuMCM::show()
    {
        VRUIPanel::show();

        // Re-center after show (centering depends on scene graph being valid)
        centerContainer();
    }

    void VRUIMenuMCM::recalculateLayout()
    {
        // Let base class position all children using VerticalDown layout
        VRUIPanel::recalculateLayout();

        // Re-center the container so it appears at the same height as the main grid
        centerContainer();
    }

    void VRUIMenuMCM::centerContainer()
    {
        if (!_container) return;

        // Center the MCM content vertically so it appears at the same position
        // as the main grid (which auto-centers via Grid layout).
        // Without this, VerticalDown layout puts everything below the hand.
        RE::NiPoint2 dims = _container->calculateLogicalDimensions();
        _container->setLocalPosition(RE::NiPoint3{ 0.0f, 0.0f, dims.y * 0.5f });
    }

    void VRUIMenuMCM::addSettingRow(const std::string& label, 
                                 const std::string& settingKey,
                                 float step,
                                 std::function<float()> getter,
                                 std::function<void(float)> setter)
    {
        // Row Layout: [-] [ LABEL : VALUE ] [+]
        auto row = std::make_shared<VRUIContainer>(_name + "_row_" + settingKey, ContainerLayout::HorizontalCenter, 0.4f);
        
        // Minus Button
        auto minusBtn = std::make_shared<VRUIButton>("Decr_" + settingKey, "immersiveUI\\slot01.nif", "textures\\test.dds", 1.2f, 0.8f);
        minusBtn->setLabel("-");
        
        // Value/Label Display (Center)
        auto labelWidget = std::make_shared<VRUIButton>("Label_" + settingKey, "immersiveUI\\slot01.nif", "textures\\test.dds", 5.5f, 0.8f);
        
        auto updateLabel = [label, labelWidget, getter]() {
            char buf[128];
            sprintf_s(buf, "%s: %.2f", label.c_str(), getter());
            labelWidget->setLabel(buf);
        };
        updateLabel();

        // Interaction logic
        minusBtn->setOnPressHandler([updateLabel, getter, setter, step](VRUIButton*) {
            setter(getter() - step);
            updateLabel();
            VRMenuManager::get().refreshActivePanels();
        });

        // Plus Button
        auto plusBtn = std::make_shared<VRUIButton>("Incr_" + settingKey, "immersiveUI\\slot01.nif", "textures\\test.dds", 1.2f, 0.8f);
        plusBtn->setLabel("+");
        plusBtn->setOnPressHandler([updateLabel, getter, setter, step](VRUIButton*) {
            setter(getter() + step);
            updateLabel();
            VRMenuManager::get().refreshActivePanels();
        });

        // Add to row in order: [-] [Display] [+]
        row->addElement(minusBtn);
        row->addElement(labelWidget);
        row->addElement(plusBtn);

        _container->addElement(row);
    }
}
