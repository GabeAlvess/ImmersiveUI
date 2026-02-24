#include "ImmersiveUI_API.h"
#include <RE/B/BSInputDeviceManager.h>
#include <RE/P/PlayerControls.h>
#include <RE/U/UIMessageQueue.h>
#include <RE/B/ButtonEvent.h>
#include <RE/U/UserEvents.h>
#include <RE/I/IFormFactory.h>
#include <RE/S/Script.h>
#include <RE/P/PlayerCharacter.h>
#include <RE/U/UI.h>
#include <RE/B/BGSSaveLoadManager.h>

#include "vrui/VRMenuManager.h"
#include "vrui/VRUIPanel.h"
#include "vrui/VRUIButton.h"
#include "vrui/VRUIToggleButton.h"
#include "vrui/VRUIContainer.h"
#include "vrui/VRUISlider.h"
#include "vrui/VRUISettings.h"
#include "vrui/VRUIMenuMCM.h"
#include "keyhandler/keyhandler.h"

using namespace vrui;

// =========================================================================
// Logger Setup
// =========================================================================

static void SetupLog()
{
    auto logsFolder = SKSE::log::log_directory();
    if (!logsFolder) {
        SKSE::stl::report_and_fail("SKSE log_directory not provided, logs disabled.");
        return;
    }

    auto logPath = *logsFolder / std::filesystem::path(Plugin::NAME) += ".log";

    auto fileLoggerPtr = std::make_shared<spdlog::sinks::basic_file_sink_mt>(logPath.string(), true);
    auto loggerPtr = std::make_shared<spdlog::logger>("log", std::move(fileLoggerPtr));

    spdlog::set_default_logger(std::move(loggerPtr));
    spdlog::set_level(spdlog::level::info);
    spdlog::flush_on(spdlog::level::info);
}

// NOTE: SKSEPlugin_Version and SKSEPlugin_Query are auto-generated
// by the commonlibsse-ng.plugin xmake rule (see xmake.lua).

// =========================================================================
// Globals
// =========================================================================
static bool g_demoMenuCreated = false;

// =========================================================================
// Demo Menu
// =========================================================================

/// Helper: creates a handler that closes the ImmersiveUI menu and opens a game menu.
static VRUIButton::PressCallback openGameMenu(const char* menuName)
{
    return [menuName](VRUIButton*) {
        VRMenuManager::get().toggleMenu();
        auto* queue = RE::UIMessageQueue::GetSingleton();
        if (queue) queue->AddMessage(menuName, RE::UI_MESSAGE_TYPE::kShow, nullptr);
    };
}

static void createDemoMenu()
{
    if (g_demoMenuCreated) {
        logger::warn("ImmersiveUI: Demo menu already created, skipping.");
        return;
    }

    logger::info("ImmersiveUI: Creating demo menu...");

    auto& manager = VRMenuManager::get();

    // --- Create Panels ---
    auto panel = std::make_shared<VRUIPanel>("MainPanel");
    auto mcmPanel = std::make_shared<VRUIMenuMCM>("MCM_Panel");
    
    mcmPanel->initializeVisuals();
    mcmPanel->setActive(false); // Start inactive
    mcmPanel->setOnBackHandler([]() {
        VRMenuManager::get().switchToPanel("MainPanel");
    });

    // --- Create Grid ---
    // The grid will hold all 36 buttons, but VRMenuManager will manage visibility per page.
    auto grid = std::make_shared<VRUIContainer>("Grid3x3", ContainerLayout::Grid, VRUISettings::get().buttonSpacing);
    grid->setPageSize(9); // Enable automatic pagination

    // Read 36 slots from INI
    for (int i = 0; i < 36; ++i) {
        auto& settings = VRUISettings::get();
        std::string action = settings.slotActions[i];
        
        // Auto-generate NIF path: meshes\slot01.nif ... slot36.nif
        // If the user specified a custom NIF in sSlotXXNif, use that instead.
        std::string nifPath = settings.slotNifs[i];
        if (nifPath.empty()) {
            char buf[64];
            sprintf_s(buf, "immersiveUI\\slot%02d.nif", i + 1);
            nifPath = buf;
        }

        std::string texturePath = settings.slotTextures[i];
        if (texturePath.empty()) {
             texturePath = "textures\\test.dds";
        }
        
        auto btn = std::make_shared<VRUIButton>(action, nifPath, texturePath, 2.0f, 2.0f);
        btn->setSlotIndex(i);
        btn->setLabel(settings.slotLabels[i]);
        btn->setSublabel(settings.slotSublabels[i]);

        // Bind functions based on string
        if (action == "NextPage" || action == "nextpage") {
            btn->setOnPressHandler([grid](VRUIButton*) {
                grid->nextPage();
                logger::info("ImmersiveUI: Switched to next page in container. Current: {}", grid->getCurrentPage());
            });
        }
        else if (action == "PrevPage" || action == "prevpage") {
            btn->setOnPressHandler([grid](VRUIButton*) {
                grid->prevPage();
                logger::info("ImmersiveUI: Switched to previous page in container. Current: {}", grid->getCurrentPage());
            });
        }
        else if (action == "Settings" || action == "settings") {
            btn->setOnPressHandler([](VRUIButton*) {
                VRMenuManager::get().switchToPanel("MCM_Panel");
            });
        }
        else if (action == "Close" || action == "close") {
            btn->setOnPressHandler([](VRUIButton*) {
                VRMenuManager::get().toggleMenu();
            });
        } 
        else if (action == "Wait" || action == "wait" || action == "Sleep" || action == "sleep") {
            btn->setOnPressHandler(openGameMenu("Sleep/Wait Menu"));
        }
        else if (action == "Journal" || action == "journal") {
            btn->setOnPressHandler(openGameMenu("Journal Menu"));
        }
        else if (action == "Map" || action == "map") {
            btn->setOnPressHandler(openGameMenu("MapMenu"));
        }
        else if (action == "Inventory" || action == "inventory") {
            btn->setOnPressHandler(openGameMenu("InventoryMenu"));
        }
        else if (action == "Magic" || action == "magic") {
            btn->setOnPressHandler(openGameMenu("MagicMenu"));
        }
        else if (action == "TweenMenu" || action == "tweenmenu") {
            btn->setOnPressHandler(openGameMenu("TweenMenu"));
        }
        else if (action == "Save" || action == "save") {
            btn->setOnPressHandler([](VRUIButton*) {
                // Toggle menu closed FIRST
                VRMenuManager::get().toggleMenu();

                // Defer the input simulation to the next frame
                auto* taskInterface = SKSE::GetTaskInterface();
                if (taskInterface) {
                    taskInterface->AddTask([]() {
                        auto* inputMgr = RE::BSInputDeviceManager::GetSingleton();
                        auto* userEvents = RE::UserEvents::GetSingleton();
                        if (inputMgr && userEvents) {
                            // F5 scancode is 0x3F. We simulate a quick tap.
                            auto* down = RE::ButtonEvent::Create(RE::INPUT_DEVICE::kKeyboard, userEvents->quicksave, 0x3F, 1.0f, 0.0f);
                            if (down) {
                                RE::InputEvent* downPtr = down;
                                inputMgr->SendEvent(&downPtr);
                            }
                            
                            auto* up = RE::ButtonEvent::Create(RE::INPUT_DEVICE::kKeyboard, userEvents->quicksave, 0x3F, 0.0f, 0.1f);
                            if (up) {
                                RE::InputEvent* upPtr = up;
                                inputMgr->SendEvent(&upPtr);
                            }
                            
                            RE::DebugNotification("ImmersiveUI: QuickSaving...");
                        }
                    });
                }
            });
        }
        else if (action.starts_with("Console:") || action.starts_with("console:") || 
                 action.starts_with("Cmd:") || action.starts_with("cmd:")) {
            size_t colonPos = action.find(':');
            std::string cmd = action.substr(colonPos + 1);
            
            btn->setOnPressHandler([cmd](VRUIButton*) {
                VRMenuManager::get().toggleMenu();
                
                // Create a temporary script to execute the command via FormFactory
                auto* script = RE::IFormFactory::Create<RE::Script>();
                if (script) {
                    script->SetCommand(cmd);
                    (void)script->CompileAndRun(RE::PlayerCharacter::GetSingleton());
                    logger::info("ImmersiveUI: Executed console command: '{}'", cmd);
                }
            });
        }
        else {
            // Unknown action (Catch-all)
            btn->setOnPressHandler([action](VRUIButton*) {
                if (action != "None" && !action.empty()) {
                    RE::DebugNotification(("ImmersiveUI: Action: " + action).c_str());
                }
            });
        }

        grid->addElement(btn);
    }

    // Assemble panel
    panel->addElement(grid);

    // Register with manager
    manager.registerPanel(panel);
    manager.registerPanel(mcmPanel);
    g_demoMenuCreated = true;

    logger::info("ImmersiveUI: Menu created with 36 slots (4 pages of 9).");
    RE::DebugNotification("ImmersiveUI: Menu Ready! Press F8 or hold LEFT grip.");

    // Do not auto-open menu here, because the player might still be in a loading screen
    // and hand node transforms are not initialized yet!
    // manager.toggleMenu();
}

// =========================================================================
// VR Input Event Sink - drives frame updates + reads controller buttons
// =========================================================================

class VRFrameUpdater : public RE::BSTEventSink<RE::InputEvent*>
{
public:
    static VRFrameUpdater* GetSingleton()
    {
        static VRFrameUpdater instance;
        return &instance;
    }

    static void Register()
    {
        auto* inputMgr = RE::BSInputDeviceManager::GetSingleton();
        if (inputMgr) {
            inputMgr->AddEventSink(GetSingleton());
            logger::info("ImmersiveUI: VRFrameUpdater input sink registered!");
        } else {
            logger::error("ImmersiveUI: Failed to get BSInputDeviceManager!");
        }
    }

    RE::BSEventNotifyControl ProcessEvent(
        RE::InputEvent* const* a_eventList,
        [[maybe_unused]] RE::BSTEventSource<RE::InputEvent*>* a_eventSource) override
    {
        // --- Calculate frame delta ---
        auto now = std::chrono::high_resolution_clock::now();
        float deltaTime = std::chrono::duration<float>(now - _lastTime).count();
        _lastTime = now;
        if (deltaTime <= 0.0f || deltaTime > 0.5f) deltaTime = 0.016f;

        // --- Drive the VR menu system each frame ---
        VRMenuManager::get().onFrameUpdate(deltaTime);

        // --- Process VR controller events ---
        if (a_eventList) {
            for (auto* event = *a_eventList; event; event = event->next) {
                if (event->eventType != RE::INPUT_EVENT_TYPE::kButton) continue;

                auto* btnEvent = event->AsButtonEvent();
                if (!btnEvent) continue;

                auto device = btnEvent->GetDevice();
                uint32_t keyCode = btnEvent->GetIDCode();

                // Identify VR controllers
                bool isVRController = false;
                bool isLeftHand = false;
                bool isRightHand = false;

#ifdef ENABLE_SKYRIM_VR
                switch (device) {
                case RE::INPUT_DEVICE::kVivePrimary:
                case RE::INPUT_DEVICE::kOculusPrimary:
                case RE::INPUT_DEVICE::kWMRPrimary:
                    isVRController = true;
                    isRightHand = true; // Primary is Right Hand
                    break;
                case RE::INPUT_DEVICE::kViveSecondary:
                case RE::INPUT_DEVICE::kOculusSecondary:
                case RE::INPUT_DEVICE::kWMRSecondary:
                    isVRController = true;
                    isLeftHand = true; // Secondary is Left Hand
                    break;
                default:
                    break;
                }
#endif

                if (isVRController) {
                    // Hand mappings explicitly tied to Settings
                    auto& settings = VRUISettings::get();
                    bool isMenuHand = settings.useLeftHandAsMenu ? isLeftHand : isRightHand;
                    bool isDominantHand = settings.useLeftHandAsMenu ? isRightHand : isLeftHand;
                    
                    // Index 2 is Right Grip, Index 7 is Left Grip (usually, can vary by headset)
                    if (isMenuHand && (keyCode == 2 || keyCode == 7)) {
                        VRMenuManager::get().onGripButtonChanged(btnEvent->IsPressed());
                    }

                    // Trigger = key 33 on most controllers
                    if (isDominantHand && keyCode == 33) {
                        VRMenuManager::get().onTriggerButtonChanged(btnEvent->IsPressed());
                    }
                }
            }
        }

        // Log first frame
        static bool firstFrame = true;
        if (firstFrame) {
            firstFrame = false;
            logger::info("ImmersiveUI: First frame - update loop ACTIVE!");
        }

        return RE::BSEventNotifyControl::kContinue;
    }

private:
    VRFrameUpdater() : _lastTime(std::chrono::high_resolution_clock::now()) {}
    std::chrono::high_resolution_clock::time_point _lastTime;
};

// =========================================================================
// SKSE Plugin Entry Point
// =========================================================================

static void SKSEMessageHandler(SKSE::MessagingInterface::Message* message)
{
    switch (message->type) {
    case SKSE::MessagingInterface::kDataLoaded:
        logger::info("ImmersiveUI: ===== kDataLoaded =====");
        VRMenuManager::get().initialize();
        VRFrameUpdater::Register();

        // Keyboard handler for F8 toggle + G grip simulation
        KeyHandler::RegisterSink();
        {
            auto* kh = KeyHandler::GetSingleton();

            // F8 = toggle menu
            kh->Register(0x42, KeyEventType::KEY_DOWN, []() {
                logger::info("ImmersiveUI: F8 -> toggle menu");
                RE::DebugNotification("ImmersiveUI: Toggle!");
                VRMenuManager::get().toggleMenu();
            });

            // G = simulate grip (hold 2s to activate)
            kh->Register(0x22, KeyEventType::KEY_DOWN, []() {
                VRMenuManager::get().onGripButtonChanged(true);
            });
            kh->Register(0x22, KeyEventType::KEY_UP, []() {
                VRMenuManager::get().onGripButtonChanged(false);
            });

            logger::info("ImmersiveUI: Keys registered (F8=toggle, G=grip)");
        }
        break;

    case SKSE::MessagingInterface::kPostLoadGame:
        logger::info("ImmersiveUI: ===== kPostLoadGame =====");
        // Create menu after game world is fully loaded (safe to load NIFs)
        if (!g_demoMenuCreated) {
            createDemoMenu();
        }
        break;

    case SKSE::MessagingInterface::kNewGame:
        logger::info("ImmersiveUI: ===== kNewGame =====");
        // Also create menu on new game
        if (!g_demoMenuCreated) {
            createDemoMenu();
        }
        break;
    }
}

extern "C" DLLEXPORT bool SKSEAPI SKSEPlugin_Load(const SKSE::LoadInterface* a_skse)
{
    // Initialize logger FIRST
    SetupLog();

    logger::info("===================================================");
    logger::info("{} v{} - LOADING", Plugin::NAME, Plugin::VERSION.string());
    logger::info("===================================================");

    REL::Module::reset();
    SKSE::Init(a_skse);
    SKSE::AllocTrampoline(1 << 10);

    auto g_messaging = reinterpret_cast<SKSE::MessagingInterface*>(
        a_skse->QueryInterface(SKSE::LoadInterface::kMessaging));

    if (!g_messaging) {
        logger::critical("ImmersiveUI: Failed to load messaging interface!");
        return false;
    }

    g_messaging->RegisterListener("SKSE", SKSEMessageHandler);
    logger::info("ImmersiveUI: Plugin loaded successfully!");

    return true;
}

// Export API for other mods
extern "C" DLLEXPORT void* RequestPluginAPI(
    [[maybe_unused]] ImmersiveUI_API::InterfaceVersion version)
{
    logger::info("ImmersiveUI: API requested (v{})", static_cast<int>(version));
    return nullptr;
}
