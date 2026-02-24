#pragma once

#include <string>

namespace vrui
{
    /// Configuration settings loaded from ImmersiveUI.ini
    struct VRUISettings
    {
        // --- Activation ---
        float activationHoldTime = 0.3f;      // Seconds to hold grip to open menu
        bool useLeftHandAsMenu = true;         // true = menu on left hand, false = right
        int activationButton = 2;              // 0=Grip, 1=Trigger, 2=Grip, 3=Thumbstick

        // --- Visual ---
        bool verboseLogging = false;        // Enable trace-level logging (very spammy, for debugging only)
        float menuScale = 0.8f;               // Overall menu scale
        float menuOffsetX = 0.0f;              // Menu offset from hand (X)
        float menuOffsetY = 10.5f;             // Menu offset from hand (Y = forward)
        float menuOffsetZ = 1.5f;             // Menu offset from hand (Z = up)
        float menuRotX = 90.0f;                 // Menu rotation X
        float menuRotY = -20.0f;                 // Menu rotation Y
        float menuRotZ = -90.0f;                 // Menu rotation Z
        
        float touchOffsetX = 0.0f;             // Touch point origin offset X (Right/Left)
        float touchOffsetY = 0.0f;             // Touch point origin offset Y (Forward/Backward)
        float touchOffsetZ = 10.0f;            // Touch point origin offset Z (Up/Down along hand direction, usually pushes to fingertips)
        
        float buttonSpacing = 3.6f;            // Space between buttons
        float buttonMeshScale = 0.02f;          // Scale of visual mesh vs logical widget
        float buttonMeshRotX = 90.0f;           // Internal mesh rotation X
        float buttonMeshRotY = 0.0f;           // Internal mesh rotation Y
        float buttonMeshRotZ = 180.0f;          // Internal mesh rotation Z
        bool flipTextureH = false;             // Flip texture UV horizontally
        bool flipTextureV = false;             // Flip texture UV vertically
        bool invertGridX = true;               // Invert columns (0 becomes 2, for viewing from back)
        
        bool showBackground = false;           // Show the background.nif
        float backgroundScale = 0.05f;
        float backgroundOffsetX = 0.0f;
        float backgroundOffsetY = -1.0f;
        float backgroundOffsetZ = -1.0f;
        float backgroundRotX = 90.0f;          // Match button mesh rotation
        float backgroundRotY = 0.0f;
        float backgroundRotZ = 180.0f;         // Match button mesh rotation

        float labelScale = 1.0f;               // 3D Text Label settings
        float labelXOffset = 0.0f;
        float labelYOffset = 0.3f;
        float labelZOffset = 0.0f;
        float labelSpacing = 0.2f;
        float labelRotX = 90.0f;
        float labelRotY = 0.0f;
        float labelRotZ = 180.0f;

        float hitboxScale = 1.0f;              // Multiplier for hitbox width/height
        float hitTestDepth = 1.0f;             // Volume depth for lasers and touch

        // --- Interaction ---
        float raycastMaxDistance = 250.0f;      // Max raycast distance
        std::string laserNifPath = "immersiveUI\\laser.nif";
        std::string backgroundNifPath = "immersiveUI\\background.nif";

        bool hapticOnHover = true;             // Haptic pulse on hover
        bool hapticOnPress = true;             // Haptic pulse on press
        float hapticIntensity = 0.5f;          // Haptic strength multiplier
        float hapticDuration = 0.04f;          // Haptic pulse duration in seconds (40ms)
        bool debugMode = false;                // Show debug info (raycast line, AABB boxes)
        
        // --- Slots (4 Pages of 9 = 36 Slots) ---
        std::string slotActions[36] = {
            "Save", "Wait", "TweenMenu",
            "Inventory", "Magic", "Map",
            "Journal", "None", "NextPage", // Page 1
            "None", "None", "None", "None", "None", "None", "None", "None", "NextPage", // Page 2
            "None", "None", "None", "None", "None", "None", "None", "None", "NextPage", // Page 3
            "None", "None", "None", "None", "None", "None", "None", "None", "NextPage"  // Page 4
        };
        std::string slotTextures[36]; // Custom icons
        std::string slotNifs[36];     // Optional custom NIFs per slot
        std::string slotLabels[36];   // 3D labels per slot
        std::string slotSublabels[36]; // 3D sublabels per slot
        
        /// Load settings from INI file
        void load(const std::string& iniPath);

        /// Save current settings to INI file
        void save(const std::string& iniPath) const;

        /// Get default INI file path (Data/SKSE/Plugins/ImmersiveUI.ini)
        static std::string getDefaultIniPath();

        /// Singleton access
        static VRUISettings& get();
    };
}
