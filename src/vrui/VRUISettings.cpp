#include "VRUISettings.h"

#include <CLIBUtil/simpleINI.hpp>
#include <filesystem>

namespace vrui
{
    VRUISettings& VRUISettings::get()
    {
        static VRUISettings instance;
        return instance;
    }

    std::string VRUISettings::getDefaultIniPath()
    {
        return "Data/SKSE/Plugins/ImmersiveUI.ini";
    }

    void VRUISettings::load(const std::string& iniPath)
    {
        CSimpleIniA ini;
        ini.SetUnicode();

        if (ini.LoadFile(iniPath.c_str()) < 0) {
            logger::info("ImmersiveUI: No INI file found at '{}', using defaults", iniPath);
            save(iniPath);  // Create default INI
            return;
        }

        logger::info("ImmersiveUI: Loading settings from '{}'", iniPath);

        // Load directly using CSimpleIniA methods to ensure proper type conversions
        activationHoldTime = ini.GetDoubleValue("Activation", "fHoldTime", activationHoldTime);
        useLeftHandAsMenu = ini.GetBoolValue("Activation", "bUseLeftHandAsMenu", useLeftHandAsMenu);
        activationButton = ini.GetLongValue("Activation", "iActivationButton", activationButton);

        verboseLogging = ini.GetBoolValue("General", "bVerboseLogging", verboseLogging);

        menuScale = ini.GetDoubleValue("Visual", "fMenuScale", menuScale);
        menuOffsetX = ini.GetDoubleValue("Visual", "fMenuOffsetX", menuOffsetX);
        menuOffsetY = ini.GetDoubleValue("Visual", "fMenuOffsetY", menuOffsetY);
        menuOffsetZ = ini.GetDoubleValue("Visual", "fMenuOffsetZ", menuOffsetZ);
        menuRotX = ini.GetDoubleValue("Visual", "fMenuRotX", menuRotX);
        menuRotY = ini.GetDoubleValue("Visual", "fMenuRotY", menuRotY);
        menuRotZ = ini.GetDoubleValue("Visual", "fMenuRotZ", menuRotZ);
        
        touchOffsetX = ini.GetDoubleValue("Visual", "fTouchOffsetX", touchOffsetX);
        touchOffsetY = ini.GetDoubleValue("Visual", "fTouchOffsetY", touchOffsetY);
        touchOffsetZ = ini.GetDoubleValue("Visual", "fTouchOffsetZ", touchOffsetZ);
        
        buttonSpacing = ini.GetDoubleValue("Visual", "fButtonSpacing", buttonSpacing);
        buttonMeshScale = ini.GetDoubleValue("Visual", "fButtonMeshScale", buttonMeshScale);
        buttonMeshRotX = ini.GetDoubleValue("Visual", "fButtonMeshRotX", buttonMeshRotX);
        buttonMeshRotY = ini.GetDoubleValue("Visual", "fButtonMeshRotY", buttonMeshRotY);
        buttonMeshRotZ = ini.GetDoubleValue("Visual", "fButtonMeshRotZ", buttonMeshRotZ);
        flipTextureH = ini.GetBoolValue("Visual", "bFlipTextureH", flipTextureH);
        flipTextureV = ini.GetBoolValue("Visual", "bFlipTextureV", flipTextureV);
        invertGridX = ini.GetBoolValue("Visual", "bInvertGridX", invertGridX);

        showBackground = ini.GetBoolValue("Visual", "bShowBackground", showBackground);
        backgroundScale = ini.GetDoubleValue("Visual", "fBackgroundScale", backgroundScale);
        backgroundOffsetX = ini.GetDoubleValue("Visual", "fBackgroundOffsetX", backgroundOffsetX);
        backgroundOffsetY = ini.GetDoubleValue("Visual", "fBackgroundOffsetY", backgroundOffsetY);
        backgroundOffsetZ = ini.GetDoubleValue("Visual", "fBackgroundOffsetZ", backgroundOffsetZ);
        backgroundRotX = ini.GetDoubleValue("Visual", "fBackgroundRotX", backgroundRotX);
        backgroundRotY = ini.GetDoubleValue("Visual", "fBackgroundRotY", backgroundRotY);
        backgroundRotZ = ini.GetDoubleValue("Visual", "fBackgroundRotZ", backgroundRotZ);

        raycastMaxDistance = ini.GetDoubleValue("Interaction", "fRaycastMaxDistance", raycastMaxDistance);
        
        labelScale = ini.GetDoubleValue("Labels", "fLabelScale", labelScale);
        labelXOffset = ini.GetDoubleValue("Labels", "fLabelXOffset", labelXOffset);
        labelYOffset = ini.GetDoubleValue("Labels", "fLabelYOffset", labelYOffset);
        labelZOffset = ini.GetDoubleValue("Labels", "fLabelZOffset", labelZOffset);
        labelSpacing = ini.GetDoubleValue("Labels", "fLabelSpacing", labelSpacing);
        labelRotX = ini.GetDoubleValue("Labels", "fLabelRotX", labelRotX);
        labelRotY = ini.GetDoubleValue("Labels", "fLabelRotY", labelRotY);
        labelRotZ = ini.GetDoubleValue("Labels", "fLabelRotZ", labelRotZ);
        laserNifPath = ini.GetValue("Interaction", "sLaserNifPath", laserNifPath.c_str());
        backgroundNifPath = ini.GetValue("Interaction", "sBackgroundNifPath", backgroundNifPath.c_str());
        hapticOnHover = ini.GetBoolValue("Interaction", "bHapticOnHover", hapticOnHover);
        hapticOnPress = ini.GetBoolValue("Interaction", "bHapticOnPress", hapticOnPress);
        hapticIntensity = ini.GetDoubleValue("Interaction", "fHapticIntensity", hapticIntensity);
        hapticDuration = ini.GetDoubleValue("Interaction", "fHapticDuration", hapticDuration);
        hitboxScale = ini.GetDoubleValue("Interaction", "fHitboxScale", hitboxScale);
        hitTestDepth = ini.GetDoubleValue("Interaction", "fHitTestDepth", hitTestDepth);

        debugMode = ini.GetBoolValue("Debug", "bDebugMode", debugMode);
        
        // Slots
        for (int i = 0; i < 36; ++i) {
            std::string keyAction = "sSlot" + std::to_string(i + 1);
            const char* valAction = ini.GetValue("Slots", keyAction.c_str(), slotActions[i].c_str());
            slotActions[i] = valAction;

            std::string keyImage = "sSlot" + std::to_string(i + 1) + "Image";
            const char* valImage = ini.GetValue("Slots", keyImage.c_str(), slotTextures[i].c_str());
            slotTextures[i] = valImage;

            std::string keyNif = "sSlot" + std::to_string(i + 1) + "Nif";
            const char* valNif = ini.GetValue("Slots", keyNif.c_str(), slotNifs[i].c_str());
            slotNifs[i] = valNif;

            std::string keyLabel = "sSlot" + std::to_string(i + 1) + "Label";
            const char* valLabel = ini.GetValue("Slots", keyLabel.c_str(), slotLabels[i].c_str());
            slotLabels[i] = valLabel;

            std::string keySublabel = "sSlot" + std::to_string(i + 1) + "Sublabel";
            const char* valSublabel = ini.GetValue("Slots", keySublabel.c_str(), slotSublabels[i].c_str());
            slotSublabels[i] = valSublabel;
        }
    }

    void VRUISettings::save(const std::string& iniPath) const
    {
        CSimpleIniA ini;
        ini.SetUnicode();

        // Activation
        ini.SetDoubleValue("Activation", "fHoldTime", activationHoldTime,
            "; Seconds to hold the activation button to toggle menu (default: 2.0)");
        ini.SetBoolValue("Activation", "bUseLeftHandAsMenu", useLeftHandAsMenu,
            "; true = menu on left hand (dominant right), false = menu on right hand");
        ini.SetLongValue("Activation", "iActivationButton", activationButton,
            "; 0=Grip, 1=Trigger, 2=Grip(default), 3=Thumbstick Press");

        // General
        ini.SetBoolValue("General", "bVerboseLogging", verboseLogging,
            "; Enable trace-level logging for debugging (default: false, very spammy)");

        // Visual
        ini.SetDoubleValue("Visual", "fMenuScale", menuScale,
            "; Overall scale of the menu panel");
        ini.SetDoubleValue("Visual", "fMenuOffsetX", menuOffsetX,
            "; Menu offset X (right/left)");
        ini.SetDoubleValue("Visual", "fMenuOffsetY", menuOffsetY,
            "; Menu offset Y (forward)");
        ini.SetDoubleValue("Visual", "fMenuOffsetZ", menuOffsetZ,
            "; Menu offset Z (up)");
        ini.SetDoubleValue("Visual", "fMenuRotX", menuRotX,
            "; Menu rotation X (pitch)");
        ini.SetDoubleValue("Visual", "fMenuRotY", menuRotY,
            "; Menu rotation Y (roll)");
        ini.SetDoubleValue("Visual", "fMenuRotZ", menuRotZ,
            "; Menu rotation Z (yaw)");
        ini.SetDoubleValue("Visual", "fTouchOffsetX", touchOffsetX,
            "; Touch point offset X (Right/Left)");
        ini.SetDoubleValue("Visual", "fTouchOffsetY", touchOffsetY,
            "; Touch point offset Y (Forward/Backward)");
        ini.SetDoubleValue("Visual", "fTouchOffsetZ", touchOffsetZ,
            "; Touch point offset Z (Push from wrist to fingertips)");
        ini.SetDoubleValue("Visual", "fButtonSpacing", buttonSpacing,
            "; Spacing between buttons");
        ini.SetDoubleValue("Visual", "fButtonMeshScale", buttonMeshScale,
            "; Scale of the button mesh (default 0.02 is for 100x100 meshes like IconPlane)");
        ini.SetDoubleValue("Visual", "fButtonMeshRotX", buttonMeshRotX,
            "; Individual button mesh rotation X");
        ini.SetDoubleValue("Visual", "fButtonMeshRotY", buttonMeshRotY,
            "; Individual button mesh rotation Y");
        ini.SetDoubleValue("Visual", "fButtonMeshRotZ", buttonMeshRotZ,
            "; Individual button mesh rotation Z");
        ini.SetBoolValue("Visual", "bFlipTextureH", flipTextureH,
            "; Flip icons horizontally (useful if you rotate the mesh 180 degrees)");
        ini.SetBoolValue("Visual", "bFlipTextureV", flipTextureV,
            "; Flip icons vertically");
        ini.SetBoolValue("Visual", "bInvertGridX", invertGridX,
            "; Invert grid columns (Horizontal flip of the layout)");
        
        ini.SetBoolValue("Visual", "bShowBackground", showBackground,
            "; Show background plane");
        ini.SetDoubleValue("Visual", "fBackgroundScale", backgroundScale,
            "; Scale of the background plane");
        ini.SetDoubleValue("Visual", "fBackgroundOffsetX", backgroundOffsetX);
        ini.SetDoubleValue("Visual", "fBackgroundOffsetY", backgroundOffsetY);
        ini.SetDoubleValue("Visual", "fBackgroundOffsetZ", backgroundOffsetZ);
        ini.SetDoubleValue("Visual", "fBackgroundRotX", backgroundRotX);
        ini.SetDoubleValue("Visual", "fBackgroundRotY", backgroundRotY);
        ini.SetDoubleValue("Visual", "fBackgroundRotZ", backgroundRotZ);

        // Interaction
        ini.SetDoubleValue("Interaction", "fRaycastMaxDistance", raycastMaxDistance,
            "; Maximum range of the interaction laser pointer");
        ini.SetValue("Interaction", "sLaserNifPath", laserNifPath.c_str(),
            "; Path to custom laser NIF");
        ini.SetValue("Interaction", "sBackgroundNifPath", backgroundNifPath.c_str(),
            "; Path to custom background NIF");
        ini.SetBoolValue("Interaction", "bHapticOnHover", hapticOnHover,
            "; Haptic pulse on button hover");
        ini.SetBoolValue("Interaction", "bHapticOnPress", hapticOnPress,
            "; Haptic pulse on button press");
        ini.SetDoubleValue("Interaction", "fHapticIntensity", hapticIntensity,
            "; Haptic strength (0-1)");
        ini.SetDoubleValue("Interaction", "fHapticDuration", hapticDuration,
            "; Haptic pulse duration in seconds");
        ini.SetDoubleValue("Interaction", "fHitboxScale", hitboxScale,
            "; Multiplier for hitbox width/height (1.0 = exact mesh size)");
        ini.SetDoubleValue("Interaction", "fHitTestDepth", hitTestDepth,
            "; Depth (thickness) of the button's selection volume");

        // Labels
        ini.SetDoubleValue("Labels", "fLabelScale", labelScale, "; Scale of characters");
        ini.SetDoubleValue("Labels", "fLabelXOffset", labelXOffset);
        ini.SetDoubleValue("Labels", "fLabelYOffset", labelYOffset);
        ini.SetDoubleValue("Labels", "fLabelZOffset", labelZOffset, "; Vertical elevation above the button");
        ini.SetDoubleValue("Labels", "fLabelSpacing", labelSpacing, "; Distance between characters in a string");
        ini.SetDoubleValue("Labels", "fLabelRotX", labelRotX);
        ini.SetDoubleValue("Labels", "fLabelRotY", labelRotY);
        ini.SetDoubleValue("Labels", "fLabelRotZ", labelRotZ);

        // Debug
        ini.SetBoolValue("Debug", "bDebugMode", debugMode,
            "; Enable debug visuals (AABB boxes, etc)");

        // Slots
        for (int i = 0; i < 36; ++i) {
            std::string keyAction = "sSlot" + std::to_string(i + 1);
            std::string commentAction = i % 9 == 0 ? "; Actions for Page " + std::to_string((i / 9) + 1) : "";
            ini.SetValue("Slots", keyAction.c_str(), slotActions[i].c_str(), commentAction.empty() ? nullptr : commentAction.c_str());

            std::string keyImage = "sSlot" + std::to_string(i + 1) + "Image";
            ini.SetValue("Slots", keyImage.c_str(), slotTextures[i].c_str());

            std::string keyNif = "sSlot" + std::to_string(i + 1) + "Nif";
            ini.SetValue("Slots", keyNif.c_str(), slotNifs[i].c_str());

            std::string keyLabel = "sSlot" + std::to_string(i + 1) + "Label";
            ini.SetValue("Slots", keyLabel.c_str(), slotLabels[i].c_str());

            std::string keySublabel = "sSlot" + std::to_string(i + 1) + "Sublabel";
            ini.SetValue("Slots", keySublabel.c_str(), slotSublabels[i].c_str());
        }

        // Ensure directory exists
        auto parentPath = std::filesystem::path(iniPath).parent_path();
        if (!parentPath.empty()) {
            std::filesystem::create_directories(parentPath);
        }

        ini.SaveFile(iniPath.c_str());
        logger::info("ImmersiveUI: Settings saved to '{}'", iniPath);
    }
}
