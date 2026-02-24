#include "VRMenuManager.h"
#include "VRUISettings.h"
#include <Windows.h>
#include <cmath>
#include <RE/B/BSVisit.h>
#include <RE/B/BSLightingShaderProperty.h>
#include <RE/B/BSLightingShaderMaterialBase.h>
#include <RE/B/BSShaderTextureSet.h>

#include <RE/B/BSOpenVR.h>
#include <RE/B/BSVRInterface.h>

namespace vrui
{
    VRMenuManager& VRMenuManager::get()
    {
        static VRMenuManager instance;
        return instance;
    }

    void VRMenuManager::initialize()
    {
        // Reset panels if already initialized (for reload support)
        if (_initialized) {
            for (auto& panel : _panels) {
                panel->detachFromParent();
            }
            _panels.clear();
        }

        // Load settings from INI
        auto& settings = VRUISettings::get();
        std::string iniPath = VRUISettings::getDefaultIniPath();
        settings.load(iniPath);

        // Apply log level based on INI setting
        if (settings.verboseLogging) {
            spdlog::set_level(spdlog::level::trace);
            logger::info("ImmersiveUI: Verbose logging ENABLED (trace level)");
        }
        
        try {
            if (std::filesystem::exists(iniPath)) {
                _lastIniModifiedTime = std::filesystem::last_write_time(iniPath);
            }
        } catch (...) {}

        // Check for VRIK explicitly because VRIK alters which player skeleton node is safely rendered (1st person vs 3rd person)
        if (GetModuleHandleA("vrik.dll") != nullptr) {
            _isVRIKInstalled = true;
            logger::info("ImmersiveUI: VRIK detected. We will attach UI to the 3rd person skeleton.");
        } else {
            _isVRIKInstalled = false;
            logger::info("ImmersiveUI: VRIK not detected. We will attach UI to the 1st person skeleton.");
        }

        // Initialize Laser Pointer mesh
        _laserPointer = VRUIWidget::loadModelFromNif(settings.laserNifPath);
        if (!_laserPointer) {
            logger::warn("ImmersiveUI: Custom laser '{}' not found, falling back to IconPlane.", settings.laserNifPath);
            _laserPointer = VRUIWidget::loadModelFromNif("ImmersiveUI\\IconPlane.nif");
        }

        if (_laserPointer) {
            // Uncull
            RE::BSVisit::TraverseScenegraphGeometries(_laserPointer.get(), [&](RE::BSGeometry* a_geometry) -> RE::BSVisit::BSVisitControl {
                a_geometry->SetAppCulled(false);
                return RE::BSVisit::BSVisitControl::kContinue;
            });
            _laserPointer->SetAppCulled(false);

            _laserPointer->local.scale = 0.0f; // Hidden by default

            // Apply texture to visible
            auto* textureSet = RE::BSShaderTextureSet::Create();
            if (textureSet) {
                textureSet->SetTexturePath(RE::BSTextureSet::Texture::kDiffuse, "textures\\test.dds"); // Just need any visible texture
                RE::BSVisit::TraverseScenegraphGeometries(_laserPointer.get(), [&](RE::BSGeometry* geom) -> RE::BSVisit::BSVisitControl {
                    if (geom) {
                        auto* shaderProp = geom->lightingShaderProp_cast();
                        if (shaderProp && shaderProp->GetBaseMaterial()) {
                            auto* material = static_cast<RE::BSLightingShaderMaterialBase*>(shaderProp->GetBaseMaterial());
                            if (material) {
                                RE::NiPointer<RE::BSTextureSet> texPtr(textureSet);
                                material->SetTextureSet(texPtr);
                            }
                        }
                    }
                    return RE::BSVisit::BSVisitControl::kContinue;
                });
            }

            logger::info("ImmersiveUI: Laser pointer mesh loaded successfully.");
        } else {
            logger::warn("ImmersiveUI: Failed to load laser pointer mesh. Laser will not be drawn.");
        }

        _initialized = true;
        logger::info("ImmersiveUI: VRMenuManager initialized");
    }

    void VRMenuManager::onFrameUpdate(float deltaTime)
    {
        if (!_initialized) return;

        // 1. Check activation input (grip hold)
        processActivationInput(deltaTime);

        // 2. If menu is open, perform touch input
        if (_menuOpen) {
            // Process touch from dominant hand
            processTouchInput(deltaTime);

            // Process trigger (button press)
            processTriggerInput();
        }

        // 3. INI reloading (Check when Journal Menu closes)
        auto* ui = RE::UI::GetSingleton();
        if (ui) {
            bool isJournalOpen = ui->IsMenuOpen("Journal Menu");
            // If the Journal was open last frame, and is now closed
            if (_wasJournalMenuOpen && !isJournalOpen) {
                std::string iniPath = VRUISettings::getDefaultIniPath();
                try {
                    if (std::filesystem::exists(iniPath)) {
                        auto newTime = std::filesystem::last_write_time(iniPath);
                        if (newTime != _lastIniModifiedTime) {
                            _lastIniModifiedTime = newTime;
                            logger::info("ImmersiveUI: INI file modification detected (after closing Pause menu), reloading settings...");
                            VRUISettings::get().load(iniPath);
                        }
                    }
                } catch (...) {}
            }
            _wasJournalMenuOpen = isJournalOpen;
        }

        // ALWAYS update panels so fade animations and hand tracking finish
        for (auto& panel : _panels) {
            if (panel) panel->update(deltaTime);
        }
    }

    void VRMenuManager::registerPanel(std::shared_ptr<VRUIPanel> panel)
    {
        _panels.push_back(std::move(panel));
        logger::info("ImmersiveUI: Panel registered (total: {})", _panels.size());
    }

    void VRMenuManager::unregisterPanel(const std::shared_ptr<VRUIPanel>& panel)
    {
        auto it = std::find(_panels.begin(), _panels.end(), panel);
        if (it != _panels.end()) {
            (*it)->hide();
            (*it)->detachFromParent();
            _panels.erase(it);
            logger::info("ImmersiveUI: Panel unregistered (total: {})", _panels.size());
        }
    }

    void VRMenuManager::switchToPanel(const std::string& panelName)
    {
        std::shared_ptr<VRUIPanel> targetPanel;
        std::shared_ptr<VRUIPanel> currentPanel;

        for (auto& p : _panels) {
            if (p->getName() == panelName) targetPanel = p;
            if (p->isActive()) currentPanel = p;
        }

        if (targetPanel && targetPanel != currentPanel) {
            RE::NiNode* handNode = getMenuHandNode();
            
            if (currentPanel) {
                currentPanel->setActive(false);
                currentPanel->hide();
                currentPanel->detachFromParent(); // Force removal from hand node
            }

            targetPanel->setActive(true);
            
            if (_menuOpen) {
                auto& settings = VRUISettings::get();
                if (handNode) {
                    targetPanel->attachToHandNode(handNode, {
                        settings.menuOffsetX,
                        settings.menuOffsetY,
                        settings.menuOffsetZ
                    });
                }
                targetPanel->recalculateLayout(); // Layout first, THEN show (so show() can apply centering)
                targetPanel->show();
            }
            
            logger::info("ImmersiveUI: Switched active panel from '{}' to '{}' (MenuOpen: {})", 
                (currentPanel ? currentPanel->getName() : "None"), panelName, _menuOpen);
        } else {
            logger::warn("ImmersiveUI: switchToPanel failed. Target '{}' not found or same as current.", panelName);
        }
    }

    void VRMenuManager::refreshActivePanels()
    {
        auto& settings = VRUISettings::get();
        for (auto& panel : _panels) {
            if (panel) {
                // Update Main Grid spacing if it exists in this panel
                auto* mainGrid = panel->findWidgetByName("Grid3x3");
                if (mainGrid) {
                    if (auto* container = dynamic_cast<VRUIContainer*>(mainGrid)) {
                        container->setSpacing(settings.buttonSpacing);
                    }
                }

                // Force a full layout refresh on the panel and its children
                panel->recalculateLayout();
                
                // If the panel is open, ensure it's attached with the latest offset/rotation/scale
                if (_menuOpen && panel->isActive() && panel->isShown()) {
                    auto* handNode = getMenuHandNode();
                    if (handNode) {
                        panel->attachToHandNode(handNode, {
                            settings.menuOffsetX,
                            settings.menuOffsetY,
                            settings.menuOffsetZ
                        });
                    }
                }
            }
        }
        logger::info("ImmersiveUI: Refreshed all active panels (Spacing: {}, Scale: {}).", 
            settings.buttonSpacing, settings.menuScale);
    }

    void VRMenuManager::toggleMenu()
    {
        _menuOpen = !_menuOpen;
        logger::info("ImmersiveUI: Menu toggled {}", _menuOpen ? "OPEN" : "CLOSED");

        auto* menuHand = getMenuHandNode();
        auto& settings = VRUISettings::get();

        for (auto& panel : _panels) {
            if (_menuOpen) {
                if (!panel->isActive()) continue; // Skip inactive panels

                if (menuHand) {
                    panel->attachToHandNode(menuHand, {
                        settings.menuOffsetX,
                        settings.menuOffsetY,
                        settings.menuOffsetZ
                    });
                }
                panel->show();
            } else {
                panel->hide();
                panel->detachFromParent(); // Guaranteed hide from scene graph

                // Clear hover state
                if (_hoveredWidget) {
                    _hoveredWidget->onRayExit();
                    if (_triggerPressed) {
                        _hoveredWidget->onTriggerRelease();
                    }
                    _hoveredWidget = nullptr;
                }
                _triggerPressed = false;
                hideLaserPointer();
            }
        }

        // Haptic feedback on toggle
        triggerHaptic(false, 0.5f, 0.2f);
    }

    // =====================================================================
    // Input Processing
    // =====================================================================

    void VRMenuManager::processActivationInput(float deltaTime)
    {
        auto& settings = VRUISettings::get();

        // VR input: Read controller button state from the player character
        // In Skyrim VR, controller input goes through the standard input system.
        // We check for button hold via PlayerControls / PlayerInputHandler
        auto* controlMap = RE::ControlMap::GetSingleton();
        if (!controlMap) return;

        // For the initial implementation, we use a keyboard fallback for testing.
        // The VR grip hold will be integrated via SkyrimVRTools API at runtime.
        // TODO: Integrate SkyrimVRTools button listener for actual VR grip detection
        
        // Temporary: use BSInputDeviceManager to poll
        // In VR, the grip button fires as a standard button event
        // We track the hold duration ourselves
        
        bool gripHeld = _gripButtonDown;  // Set by external input callback

        if (gripHeld) {
            _gripHoldTimer += deltaTime;
            if (_gripHoldTimer >= settings.activationHoldTime && !_gripWasHeld) {
                _gripWasHeld = true;
                toggleMenu();
            }
        } else {
            _gripHoldTimer = 0.0f;
            _gripWasHeld = false;
        }
    }

    void VRMenuManager::processTouchInput(float deltaTime)
    {
        auto* dominantHand = getDominantHandNode();
        if (!dominantHand) return;

        auto& settings = VRUISettings::get();

        // Tick down the hover lock timer
        if (_hoverLockTimer > 0.0f) {
            _hoverLockTimer -= deltaTime;
        }

        // Collect buttons ONLY from the active and shown panel to avoid overlapping hits
        // Force-update panel world transforms first to prevent hitbox drift during
        // player movement (walk/run/jump animations offset the skeleton bone positions)
        std::vector<VRUIButton*> allButtons;
        for (auto& panel : _panels) {
            if (panel->isActive() && panel->isShown()) {
                // Force-refresh world transforms from current VR hand position
                if (panel->getNode()) {
                    RE::NiUpdateData ud;
                    ud.flags = RE::NiUpdateData::Flag::kDirty;
                    panel->getNode()->Update(ud);
                }
                panel->collectButtons(allButtons);
            }
        }

        RE::NiPoint3 rayOrigin = dominantHand->world.translate;
        
        // Skyrim's forward axis for hand/weapon nodes is Z (Col 2).
        RE::NiMatrix3& rot = dominantHand->world.rotate;
        RE::NiPoint3 rayDir(rot.entry[0][2], rot.entry[1][2], rot.entry[2][2]);
        
        // Find which widget the ray intersects
        VRUIWidget* touchedWidget = nullptr;
        float closestDist = settings.raycastMaxDistance; 
        
        for (auto* widget : allButtons) {
            if (!widget || !widget->isVisible()) continue;
            
            float hitDist = 0.0f;
            if (widget->hitTest(rayOrigin, rayDir, hitDist)) {
                if (hitDist > 0.0f && hitDist < closestDist) {
                    closestDist = hitDist;
                    touchedWidget = widget;
                }
            }
        }

        // --- Hover Hysteresis (prevents flickering) ---
        if (_hoveredWidget) {
            if (touchedWidget == _hoveredWidget) {
                // Still hovering the same widget: keep refreshing the lock timer
                // so it never expires while the ray is consistently on the button.
                _hoverLockTimer = kHoverLockTime;
            }
            else if (_hoverLockTimer > 0.0f) {
                // Ray moved off the current widget (to nullptr or another widget)
                // but the lock timer hasn't expired yet — keep the current hover.
                // This prevents the feedback loop where setState()->scale changes 
                // momentarily push the ray outside the hitbox.
                touchedWidget = _hoveredWidget;
            }
        }

        // Handle hover state changes
        if (touchedWidget != _hoveredWidget) {
            // Exit previous hover
            if (_hoveredWidget) {
                _hoveredWidget->onRayExit();
            }

            // Enter new hover
            _hoveredWidget = touchedWidget;
            if (_hoveredWidget) {
                _hoveredWidget->onRayEnter();
                _hoverLockTimer = kHoverLockTime; // Start lock timer on new hover
                if (settings.hapticOnHover) {
                    triggerHaptic(true, settings.hapticIntensity * 0.5f, settings.hapticDuration);
                }
            }
        }

        // Update Laser pointer visually
        updateLaserPointer(dominantHand, closestDist);
    }

    void VRMenuManager::processTriggerInput()
    {
        auto& settings = VRUISettings::get();

        // Trigger state is set by external input callback
        bool triggerNowPressed = _triggerButtonDown;

        if (triggerNowPressed && !_triggerPressed) {
            // Trigger just pressed
            _triggerPressed = true;
            if (_hoveredWidget) {
                _hoveredWidget->onTriggerPress();
                if (settings.hapticOnPress) {
                    triggerHaptic(true, settings.hapticIntensity, settings.hapticDuration);
                }
            }
        } else if (!triggerNowPressed && _triggerPressed) {
            // Trigger just released
            _triggerPressed = false;
            if (_hoveredWidget) {
                _hoveredWidget->onTriggerRelease();
            }
        }
    }

    // =====================================================================
    // External Input Callbacks (called by SkyrimVRTools or KeyHandler)
    // =====================================================================

    void VRMenuManager::onGripButtonChanged(bool pressed)
    {
        _gripButtonDown = pressed;
    }

    void VRMenuManager::onTriggerButtonChanged(bool pressed)
    {
        _triggerButtonDown = pressed;
    }

    // =====================================================================
    // Hand Node Discovery
    // =====================================================================

    RE::NiNode* VRMenuManager::getMenuHandNode() const
    {
        auto& settings = VRUISettings::get();
        auto* player = RE::PlayerCharacter::GetSingleton();
        if (!player) return nullptr;

        auto* root = player->Get3D(!_isVRIKInstalled);
        if (!root) return nullptr;

        // Find the hand bone in the player skeleton
        const char* handBone = settings.useLeftHandAsMenu
            ? "NPC L Hand [LHnd]"
            : "NPC R Hand [RHnd]";

        auto* obj = root->GetObjectByName(handBone);
        return obj ? obj->AsNode() : nullptr;
    }

    RE::NiNode* VRMenuManager::getDominantHandNode() const
    {
        auto& settings = VRUISettings::get();
        auto* player = RE::PlayerCharacter::GetSingleton();
        if (!player) return nullptr;

        auto* root = player->Get3D(!_isVRIKInstalled);
        if (!root) return nullptr;

        // Dominant hand is opposite of menu hand
        const char* handBone = settings.useLeftHandAsMenu
            ? "NPC R Hand [RHnd]"
            : "NPC L Hand [LHnd]";

        auto* obj = root->GetObjectByName(handBone);
        return obj ? obj->AsNode() : nullptr;
    }

    RE::NiNode* VRMenuManager::getPlayerSkeletonRoot() const
    {
        auto* player = RE::PlayerCharacter::GetSingleton();
        if (!player) return nullptr;

        auto* root3d = player->Get3D(!_isVRIKInstalled);
        if (!root3d) return nullptr;
        return root3d->AsNode();
    }

    // =====================================================================
    // Haptic Feedback
    // =====================================================================

    void VRMenuManager::triggerHaptic(
        bool isDominantHand,
        float intensity,
        float duration)
    {
#ifdef ENABLE_SKYRIM_VR
        auto* openVR = RE::BSOpenVR::GetSingleton();
        if (!openVR) return;

        auto& settings = VRUISettings::get();
        
        // Map logical hand to physical controller
        bool doRightController;
        if (settings.useLeftHandAsMenu) {
            // Menu on Left, Dominant (Laser) on Right
            doRightController = isDominantHand;
        } else {
            // Menu on Right, Dominant (Laser) on Left
            doRightController = !isDominantHand;
        }

        // TriggerHapticPulse duration: 250 = 1.0 second (4ms units)
        // Scale by intensity to allow the INI setting to work
        float gameDuration = duration * 250.0f * intensity;
        
        if (gameDuration > 0.0f) {
            openVR->TriggerHapticPulse(doRightController, gameDuration);
        }
#endif
    }

    // =====================================================================
    // Laser Pointer
    // =====================================================================

    void VRMenuManager::updateLaserPointer(RE::NiNode* dominantHand, float targetDistance)
    {
        if (!_laserPointer || !dominantHand) return;

        // Smooth target distance to reduce sensitivity/jitter in length
        float deltaTime = 0.011f; // assume 90hz fallback
        float lerpSpeed = 15.0f; 
        float t = 1.0f - std::exp(-lerpSpeed * deltaTime);
        if (t > 1.0f) t = 1.0f;
        if (t < 0.0f) t = 0.0f;


        // -------------------------------------------------------------
        // NEW PARADIGM: True VR Rigid Laser Pointer
        // -------------------------------------------------------------
        // The laser must NEVER use LookAt math to aim at the button. 
        // Like a real laser pointer, it must rigidly point straight out 
        // of the controller. We simply stretch its length to hit the button.
        
        // Let's smooth the length
        if (!_laserActive) {
            _laserLastDist = targetDistance;
            dominantHand->AttachChild(_laserPointer.get());
            _laserActive = true;
        }

        float smoothDist = _laserLastDist + (targetDistance - _laserLastDist) * t;
        _laserLastDist = smoothDist;
        
        float halfDist = smoothDist * 0.5f;

        // In Skyrim VR, the controller's "forward" is the local Z axis (Col 2).
        // Since IconPlane.nif is flat, its geometric origin is likely in the center (from -1 to 1).
        // Scaling it by halfDist stretches it uniformly from -halfDist to +halfDist.
        // Therefore, we must translate it by halfDist along Z so it starts exactly at 0 (the controller).
        
        // Translation: Move the center of the beam forward by half its length 
        // along the local Z axis so it starts exactly at the controller tip.
        _laserPointer->local.translate.x = 0.0f;
        _laserPointer->local.translate.y = 0.0f;
        _laserPointer->local.translate.z = halfDist;

        // Apply Non-uniform Scale by overwriting the local rotation matrix
        float thickness = 0.015f; 
        _laserPointer->local.scale = 1.0f; // Reset base uniform scale
        
        // Col0 (X) maps to Controller X (Right) -> thickness
        _laserPointer->local.rotate.entry[0][0] = thickness;
        _laserPointer->local.rotate.entry[1][0] = 0.0f;
        _laserPointer->local.rotate.entry[2][0] = 0.0f;

        // Col1 (Y) maps to Controller Y (Up) -> thickness
        _laserPointer->local.rotate.entry[0][1] = 0.0f;
        _laserPointer->local.rotate.entry[1][1] = thickness;
        _laserPointer->local.rotate.entry[2][1] = 0.0f;

        // Col2 (Z) maps to Controller Z (Forward) -> length stretch
        _laserPointer->local.rotate.entry[0][2] = 0.0f;
        _laserPointer->local.rotate.entry[1][2] = 0.0f;
        _laserPointer->local.rotate.entry[2][2] = halfDist;

        // Force update to recalculate world transforms immediately
        RE::NiUpdateData ctx;
        ctx.time = 0.0f;
        ctx.flags = RE::NiUpdateData::Flag::kDirty;
        _laserPointer->Update(ctx);
    }

    void VRMenuManager::hideLaserPointer()
    {
        if (_laserPointer && _laserActive) {
            // Find parent to detach from, which should be the dominant hand, but to be safe, get its actual parent
            auto* parent = _laserPointer->parent;
            if (parent) {
                parent->DetachChild(_laserPointer.get()); // Removes from scene graph
            }
            _laserActive = false;
        }
    }

    RE::NiPoint3 VRMenuManager::getLaserOrigin() const
    {
        auto* dominantHand = getDominantHandNode();
        return dominantHand ? dominantHand->world.translate : RE::NiPoint3();
    }

    RE::NiPoint3 VRMenuManager::getLaserDirection() const
    {
        auto* dominantHand = getDominantHandNode();
        if (!dominantHand) return RE::NiPoint3(0, 0, 1);

        RE::NiMatrix3& rot = dominantHand->world.rotate;
        return RE::NiPoint3(rot.entry[0][2], rot.entry[1][2], rot.entry[2][2]);
    }
}
