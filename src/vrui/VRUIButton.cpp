#include "VRUIButton.h"
#include <RE/B/BSLightingShaderProperty.h>
#include <RE/B/BSLightingShaderMaterial.h>
#include <RE/B/BSLightingShaderMaterialBase.h>
#include <RE/B/BSShaderTextureSet.h>
#include <RE/N/NiAlphaProperty.h>
#include <RE/B/BSEffectShaderProperty.h>
#include <RE/B/BSEffectShaderMaterial.h>
#include <RE/B/BSShaderTextureSet.h>
#include <RE/B/BSVisit.h>
#include <RE/N/NiNode.h>
#include "VRUISettings.h"

namespace vrui
{
    VRUIButton::VRUIButton(const std::string& label, float width, float height)
        : VRUIWidget(label, width, height)
        , _label(label)
        , _sublabel("")
        , _state(ButtonState::Normal)
        , _slotIndex(-1)
    {
        // vtable is fully set up here, safe to call virtual
        initializeVisuals();
    }

    VRUIButton::VRUIButton(const std::string& label, const std::string& nifPath, const std::string& texturePath, float width, float height)
        : VRUIWidget(label, width, height)
        , _label(label)
        , _sublabel("")
        , _nifPath(nifPath)
        , _texturePath(texturePath)
        , _state(ButtonState::Normal)
        , _slotIndex(-1)
    {
        // vtable is fully set up here, safe to call virtual
        initializeVisuals();
    }

    void VRUIButton::initializeVisuals()
    {
        // Base node already created by VRUIWidget constructor.
        // Now load visual meshes (vtable is ready at this point).

        if (!_nifPath.empty()) {
            // Try loading custom NIF mesh via BSModelDB
            auto loaded = loadModelFromNif(_nifPath);
            if (loaded && _node) {
                auto& settings = VRUISettings::get();
                float radX = settings.buttonMeshRotX * (kDegToRad);
                float radY = settings.buttonMeshRotY * (kDegToRad);
                float radZ = settings.buttonMeshRotZ * (kDegToRad);
                loaded->local.rotate.SetEulerAnglesXYZ(radX, radY, radZ);

                RE::NiUpdateData updateData;
                loaded->Update(updateData);

                // Apply visual scale from settings
                loaded->local.scale = settings.buttonMeshScale;

                _node->AttachChild(loaded.get());
                logger::info("ImmersiveUI: Button '{}' loaded NIF '{}' with scale {} and rotation [{}, {}, {}]", 
                    _label, _nifPath, settings.buttonMeshScale, settings.buttonMeshRotX, settings.buttonMeshRotY, settings.buttonMeshRotZ);
                return;
            }
        }

        // Use a game mesh as a visible button placeholder
        // Load the flat plane / marker mesh from game files
        RE::NiPointer<RE::NiNode> meshNode;
        RE::BSModelDB::DBTraits::ArgsType args{};

        // Try several common game meshes, with and without prefixes and different slashes
        // Try several common game meshes, with and without prefixes and different slashes
        static const char* meshPaths[] = {
            "immersiveUI\\IconPlane.nif", 
            "ImmersiveUI\\IconPlane.nif",
            "meshes\\immersiveUI\\IconPlane.nif",
            "meshes\\PipboyConfigHUDv2.nif",
            "PipboyConfigHUDv2.nif",
            "FRIK\\PipboyConfigHUDv2.nif",
            "clutter\\common\\bucket01.nif",
            "meshes\\clutter\\common\\bucket01.nif",
            "clutter/common/bucket01.nif",
            "meshes/clutter/common/bucket01.nif",
            "markers\\movemarker01.nif",
            "Sky\\skyrim_moon_v2.nif", // very common base mesh
            "Sky\\Secunda.nif",
            "weapons\\iron\\longsword.nif"
        };

        auto& settings = VRUISettings::get();
        bool loaded = false;
        for (auto* path : meshPaths) {
            auto result = RE::BSModelDB::Demand(path, meshNode, args);
            logger::info("ImmersiveUI: BSModelDB::Demand test path '{}' result={}", path, static_cast<int>(result));
            
            if (result == RE::BSResource::ErrorCode::kNone && meshNode) {
                auto* cloned = meshNode->Clone();
                if (cloned && _node) {
                    auto* cloneNode = cloned->AsNode();
                    if (cloneNode) {
                        // Uncull and apply basic transform
                        RE::BSVisit::TraverseScenegraphGeometries(cloneNode, [&](RE::BSGeometry* a_geometry) -> RE::BSVisit::BSVisitControl {
                            a_geometry->SetAppCulled(false);
                            return RE::BSVisit::BSVisitControl::kContinue;
                        });
                        cloneNode->SetAppCulled(false);
                        
                        // Apply rotation from settings
                        float radX = settings.buttonMeshRotX * (kDegToRad);
                        float radY = settings.buttonMeshRotY * (kDegToRad);
                        float radZ = settings.buttonMeshRotZ * (kDegToRad);
                        cloneNode->local.rotate.SetEulerAnglesXYZ(radX, radY, radZ);

                        // Apply visual scale from settings
                        cloneNode->local.scale = settings.buttonMeshScale;
                        
                        RE::NiUpdateData updateData;
                        cloneNode->Update(updateData);
                        _node->AttachChild(cloneNode);
                        
                        loaded = true;
                        logger::info("ImmersiveUI: Button '{}' using game mesh '{}'", _label, path);
                        break;
                    }
                }
            }
        }
        if (!loaded) {
            logger::warn("ImmersiveUI: Button '{}' has no visual mesh (all load attempts failed)", _label);
        }

        // Apply custom texture if provided
        if (!_texturePath.empty() && _node) {
            auto* textureSet = RE::BSShaderTextureSet::Create();
            if (textureSet) {
                textureSet->SetTexturePath(RE::BSTextureSet::Texture::kDiffuse, _texturePath.c_str());

                bool textureApplied = false;

                // Find geometries to apply the texture and fix transparency
                RE::BSVisit::TraverseScenegraphGeometries(_node.get(), [&](RE::BSGeometry* geom) -> RE::BSVisit::BSVisitControl {
                    if (geom) {
                        // 1. Fix Transparency (NiAlphaProperty)
                        auto& runtimeData = geom->GetGeometryRuntimeData();
                        auto* alphaProp = netimmerse_cast<RE::NiAlphaProperty*>(runtimeData.properties[RE::BSGeometry::States::kProperty].get());
                        if (alphaProp) {
                            alphaProp->SetAlphaBlending(true);
                            alphaProp->SetAlphaTesting(false);
                            alphaProp->SetSrcBlendMode(RE::NiAlphaProperty::AlphaFunction::kSrcAlpha);
                            alphaProp->SetDestBlendMode(RE::NiAlphaProperty::AlphaFunction::kInvSrcAlpha);
                        }

                        // 2. Apply Texture and Fix Shader Flags
                        auto* lightingProp = geom->lightingShaderProp_cast();
                        if (lightingProp) {
                            // SLSF1_Vertex_Alpha (3rd bit in EShaderPropertyFlag8) is required for transparency in many cases
                            lightingProp->SetFlags(RE::BSShaderProperty::EShaderPropertyFlag8::kVertexAlpha, true);
                            
                            if (lightingProp->GetBaseMaterial()) {
                                auto* material = static_cast<RE::BSLightingShaderMaterialBase*>(lightingProp->GetBaseMaterial());
                                if (material) {
                                    RE::NiPointer<RE::BSTextureSet> texPtr(textureSet);
                                    material->SetTextureSet(texPtr);
                                    
                                    // Apply UV flipping if requested
                                    auto& settings = VRUISettings::get();
                                    if (settings.flipTextureH) {
                                        material->texCoordScale[0].x = -1.0f;
                                        material->texCoordOffset[0].x = 1.0f;
                                    }
                                    if (settings.flipTextureV) {
                                        material->texCoordScale[0].y = -1.0f;
                                        material->texCoordOffset[0].y = 1.0f;
                                    }
                                    
                                    textureApplied = true;
                                }
                            }
                        }

                        // Support for BSEffectShaderProperty (common for UI/Transparent elements)
                        auto* effect = runtimeData.properties[RE::BSGeometry::States::kEffect].get();
                        if (effect) {
                            auto* effectProp = netimmerse_cast<RE::BSEffectShaderProperty*>(effect);
                            if (effectProp && effectProp->GetMaterial()) {
                                effectProp->GetMaterial()->sourceTexturePath = _texturePath;
                                textureApplied = true;
                            }
                        }
                    }
                    return RE::BSVisit::BSVisitControl::kContinue;
                });
                
                if (textureApplied) {
                    logger::info("ImmersiveUI: Button '{}' applied custom texture '{}'", _label, _texturePath);
                } else {
                    logger::warn("ImmersiveUI: Button '{}' failed to apply texture '{}' (No BSLightingShaderProperty found on your custom NIF!)", _label, _texturePath);
                }
            } else {
                logger::error("ImmersiveUI: Button '{}' failed to create BSShaderTextureSet '{}'", _label, _texturePath);
            }
        }

        // Log the resulting node hierarchy for debugging
        logNodeHierarchy("Button '" + _label + "' after initializeVisuals");

        // Refresh 3D labels if they exist
        if (!_label.empty() || !_sublabel.empty()) {
            refreshLabel();
        }
    }

    void VRUIButton::refreshLabel()
    {
        if (!_node) return;

        auto& settings = VRUISettings::get();

        // 1. Clear existing label nodes safely
        if (_labelNode) {
            _node->DetachChild(_labelNode.get());
            _labelNode = nullptr;
        }
        if (_sublabelNode) {
            _node->DetachChild(_sublabelNode.get());
            _sublabelNode = nullptr;
        }

        if (_label.empty() && _sublabel.empty()) return;

        RE::NiUpdateData updateData;

        // 2. Build 3D Label
        if (!_label.empty()) {
            _labelNode = RE::NiPointer<RE::NiNode>(RE::NiNode::Create());
            _labelNode->name = "LabelContainer";
            _node->AttachChild(_labelNode.get());

            float currentX = 0.0f;
            std::vector<RE::NiPointer<RE::NiAVObject>> charNodes;

            for (char c : _label) {
                if (c == ' ' || c == '\t') {
                    currentX += settings.labelSpacing * 2.0f;
                    continue;
                }

                char upperC = static_cast<char>(toupper(static_cast<unsigned char>(c)));
                std::string charNifPath = "immersiveUI\\font\\" + std::string(1, upperC) + ".nif";

                auto charModel = loadModelFromNif(charNifPath);
                if (charModel) {
                    charModel->local.translate.x = currentX;
                    charModel->local.scale = settings.labelScale;
                    _labelNode->AttachChild(charModel.get());
                    charNodes.push_back(charModel);
                    currentX += settings.labelSpacing;
                } else {
                    currentX += settings.labelSpacing * 0.5f;
                }
            }

            if (!charNodes.empty()) {
                float totalWidth = currentX - settings.labelSpacing;
                float centerOffset = -totalWidth / 2.0f;
                for (auto& node : charNodes) {
                    node->local.translate.x += centerOffset;
                }
                _labelNode->local.translate.x = settings.labelXOffset;
                _labelNode->local.translate.y = settings.labelYOffset;
                _labelNode->local.translate.z = settings.labelZOffset;

                float radX = settings.labelRotX * (kDegToRad);
                float radY = settings.labelRotY * (kDegToRad);
                float radZ = settings.labelRotZ * (kDegToRad);
                _labelNode->local.rotate.SetEulerAnglesXYZ(radX, radY, radZ);
            }
            _labelNode->Update(updateData);
        }

        // 3. Build 3D Sublabel
        if (!_sublabel.empty()) {
            _sublabelNode = RE::NiPointer<RE::NiNode>(RE::NiNode::Create());
            _sublabelNode->name = "SublabelContainer";
            _node->AttachChild(_sublabelNode.get());

            float subX = 0.0f;
            std::vector<RE::NiPointer<RE::NiAVObject>> subNodes;
            float subScale = settings.labelScale * 0.7f;

            for (char c : _sublabel) {
                if (c == ' ' || c == '\t') {
                    subX += settings.labelSpacing * 1.5f;
                    continue;
                }
                char upperC = static_cast<char>(toupper(static_cast<unsigned char>(c)));
                std::string subNifPath = "immersiveUI\\font\\" + std::string(1, upperC) + ".nif";

                auto subModel = loadModelFromNif(subNifPath);
                if (subModel) {
                    subModel->local.translate.x = subX;
                    subModel->local.scale = subScale;
                    _sublabelNode->AttachChild(subModel.get());
                    subNodes.push_back(subModel);
                    subX += settings.labelSpacing * 0.7f;
                }
            }

            if (!subNodes.empty()) {
                float subTotalWidth = subX - (settings.labelSpacing * 0.7f);
                float subCenterOffset = -subTotalWidth / 2.0f;
                for (auto& node : subNodes) {
                    node->local.translate.x += subCenterOffset;
                }
                _sublabelNode->local.translate.x = settings.labelXOffset;
                _sublabelNode->local.translate.y = settings.labelYOffset;
                _sublabelNode->local.translate.z = settings.labelZOffset - 0.5f;

                float radX = settings.labelRotX * (kDegToRad);
                float radY = settings.labelRotY * (kDegToRad);
                float radZ = settings.labelRotZ * (kDegToRad);
                _sublabelNode->local.rotate.SetEulerAnglesXYZ(radX, radY, radZ);
            }
            _sublabelNode->Update(updateData);
        }

        logger::info("ImmersiveUI: Refreshed 3D labels for button '{}' (L='{}', S='{}')", _label, _label, _sublabel);
    }

    void VRUIButton::setState(ButtonState newState)
    {
        if (_state == newState) return;

        auto oldState = _state;
        _state = newState;

        // Set target scale for smooth interpolation (actual scale applied in update())
        switch (newState) {
        case ButtonState::Normal:
            _targetScale = 1.0f;
            break;
        case ButtonState::Hovered:
            _targetScale = 1.1f;
            break;
        case ButtonState::Pressed:
            _targetScale = 0.9f;
            break;
        }

        logger::trace("ImmersiveUI: Button '{}' state: {} -> {}",
            _label, static_cast<int>(oldState), static_cast<int>(newState));
    }

    void VRUIButton::update(float deltaTime)
    {
        // Smooth scale interpolation (prevents hitbox flicker from instant scale jumps)
        if (_node && fabsf(_currentScale - _targetScale) > 0.001f) {
            float lerpSpeed = 10.0f; // Higher = faster response
            float blend = deltaTime * lerpSpeed;
            if (blend > 1.0f) blend = 1.0f;
            _currentScale += (_targetScale - _currentScale) * blend;
            _node->local.scale = _currentScale;
        }

        // Call base class update (handles children + entrance animation)
        VRUIWidget::update(deltaTime);
    }

    void VRUIButton::onRayEnter()
    {
        if (_state != ButtonState::Pressed) {
            setState(ButtonState::Hovered);
        }
        if (_onHoverHandler) {
            _onHoverHandler(this, true);
        }
        logger::trace("ImmersiveUI: [HOVER ENTER] Button '{}'", _label);
    }

    void VRUIButton::onRayExit()
    {
        if (_state != ButtonState::Pressed) {
            setState(ButtonState::Normal);
        }
        if (_onHoverHandler) {
            _onHoverHandler(this, false);
        }
        logger::trace("ImmersiveUI: [HOVER EXIT] Button '{}'", _label);
    }

    void VRUIButton::onTriggerPress()
    {
        setState(ButtonState::Pressed);
        if (_onPressHandler) {
            _onPressHandler(this);
        }
        logger::info("ImmersiveUI: [PRESS] Button '{}'", _label);
    }

    void VRUIButton::onTriggerRelease()
    {
        setState(ButtonState::Normal);
        if (_onReleaseHandler) {
            _onReleaseHandler(this);
        }
        logger::info("ImmersiveUI: [RELEASE] Button '{}'", _label);
    }
}
