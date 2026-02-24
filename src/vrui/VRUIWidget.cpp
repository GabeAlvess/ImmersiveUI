#include "VRUIWidget.h"
#include "VRUISettings.h"
#include <RE/Skyrim.h>
#include <CLIBUtil/numeric.hpp>
#include <RE/B/BSEffectShaderProperty.h>
#include <RE/B/BSEffectShaderMaterial.h>
#include <RE/B/BSShaderProperty.h>
#include <RE/N/NiAlphaProperty.h>
#include <RE/B/BSModelDB.h>
#include <RE/B/BSVisit.h>
#include <RE/B/BSGeometry.h>
#include <RE/N/NiSmartPointer.h>
#include <RE/N/NiColor.h>

#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif

namespace vrui
{
    std::map<std::string, RE::NiPointer<RE::NiNode>> VRUIWidget::_nifCache;

    // =====================================================================
    // AABB
    // =====================================================================

    bool AABB::intersectsRay(const RE::NiPoint3& origin, const RE::NiPoint3& direction, float& outDistance) const
    {
        // Slab method for ray-AABB intersection
        float tmin = -std::numeric_limits<float>::infinity();
        float tmax = std::numeric_limits<float>::infinity();

        float axes[3] = { direction.x, direction.y, direction.z };
        float origins[3] = { origin.x, origin.y, origin.z };
        float mins[3] = { min.x, min.y, min.z };
        float maxs[3] = { max.x, max.y, max.z };

        for (int i = 0; i < 3; ++i) {
            if (std::abs(axes[i]) < 1e-8f) {
                if (origins[i] < mins[i] || origins[i] > maxs[i]) {
                    return false;
                }
            } else {
                float invD = 1.0f / axes[i];
                float t1 = (mins[i] - origins[i]) * invD;
                float t2 = (maxs[i] - origins[i]) * invD;
                if (t1 > t2) std::swap(t1, t2);
                tmin = (tmin > t1) ? tmin : t1;
                tmax = (tmax < t2) ? tmax : t2;
                if (tmin > tmax) {
                    return false;
                }
            }
        }

        outDistance = tmin >= 0 ? tmin : tmax;
        return outDistance >= 0;
    }

    // =====================================================================
    // VRUIWidget
    // =====================================================================

    VRUIWidget::VRUIWidget(const std::string& name, float width, float height)
        : _name(name), _width(width), _height(height)
    {
        _baseScale = 1.0f;
        _animProgress = 1.0f;
        _animDelayFrames = 0;
        createNode();
    }

    VRUIWidget::~VRUIWidget()
    {
        detachFromParent();
    }

    void VRUIWidget::addChild(std::shared_ptr<VRUIWidget> child)
    {
        if (child->_parent) {
            child->detachFromParent();
        }
        child->_parent = this;
        _children.push_back(child);

        if (_node && child->_node) {
            _node->AttachChild(child->_node.get());
        }
    }

    void VRUIWidget::removeChild(const std::shared_ptr<VRUIWidget>& child)
    {
        auto it = std::find(_children.begin(), _children.end(), child);
        if (it != _children.end()) {
            child->_parent = nullptr;
            if (_node && child->_node) {
                _node->DetachChild(child->_node.get());
            }
            _children.erase(it);
        }
    }

    void VRUIWidget::attachToNode(RE::NiNode* parent)
    {
        if (parent && _node) {
            parent->AttachChild(_node.get());
            RE::NiUpdateData updateData;
            _node->Update(updateData);
            logger::info("ImmersiveUI: Widget '{}' attached to node '{}'",
                _name, parent->name.c_str());
        }
    }

    void VRUIWidget::detachFromParent()
    {
        if (_node && _node->parent) {
            _node->parent->DetachChild(_node.get());
        }
    }

    void VRUIWidget::setLocalPosition(const RE::NiPoint3& pos)
    {
        if (_node) {
            _node->local.translate = pos;
            RE::NiUpdateData updateData;
            _node->Update(updateData);
        }
    }

    RE::NiPoint3 VRUIWidget::getLocalPosition() const
    {
        return _node ? _node->local.translate : RE::NiPoint3{ 0.0f, 0.0f, 0.0f };
    }


    void VRUIWidget::setLocalScale(float scale)
    {
        _baseScale = scale;
        if (_node) {
            // If we're not currently in the middle of an animation, apply immediately
            if (_animProgress >= 1.0f) {
                _node->local.scale = scale;
                RE::NiUpdateData updateData;
                _node->Update(updateData);
            }
        }
    }

    float VRUIWidget::getLocalScale() const
    {
        return _node ? _node->local.scale : _baseScale;
    }

    VRUIWidget* VRUIWidget::findWidgetByName(const std::string& name)
    {
        if (_name == name) return this;
        for (auto& child : _children) {
            auto* found = child->findWidgetByName(name);
            if (found) return found;
        }
        return nullptr;
    }

    bool VRUIWidget::isVisible() const
    {
        if (!_visible) return false;
        if (_parent) return _parent->isVisible();
        return true;
    }

    RE::NiPoint2 VRUIWidget::calculateLogicalDimensions() const
    {
        return { _width, _height };
    }
    
    void VRUIWidget::setLocalRotation(const RE::NiMatrix3& rot)
    {
        if (_node) {
            _node->local.rotate = rot;
            RE::NiUpdateData updateData;
            _node->Update(updateData);
        }
    }

    RE::NiPoint3 VRUIWidget::getWorldPosition() const
    {
        if (_node) {
            return _node->world.translate;
        }
        return {};
    }

    void VRUIWidget::setVisible(bool visible)
    {
        _visible = visible;
        if (_node) {
            _node->SetAppCulled(!visible);
        }
    }

    AABB VRUIWidget::getWorldAABB() const
    {
        AABB box;
        if (_node) {
            auto pos = _node->world.translate;
            float halfW = _width * _node->world.scale * 0.5f;
            float halfH = _height * _node->world.scale * 0.5f;
            // Menu panel is in XZ plane relative to hand, with Y as depth
            box.min = { pos.x - halfW, pos.y - 0.5f, pos.z - halfH };
            box.max = { pos.x + halfW, pos.y + 0.5f, pos.z + halfH };
        }
        return box;
    }

    bool VRUIWidget::hitTest(const RE::NiPoint3& rayOriginWorld, const RE::NiPoint3& rayDirWorld, float& outDistance) const
    {
        if (!_node) return false;

        const auto& t = _node->world;

        // 1. Transform ray Origin to Node Local Space
        RE::NiPoint3 diff;
        diff.x = rayOriginWorld.x - t.translate.x;
        diff.y = rayOriginWorld.y - t.translate.y;
        diff.z = rayOriginWorld.z - t.translate.z;

        // Apply Transpose of rotation matrix (which is its inverse)
        RE::NiPoint3 localOrigin;
        localOrigin.x = (t.rotate.entry[0][0]*diff.x + t.rotate.entry[1][0]*diff.y + t.rotate.entry[2][0]*diff.z) / t.scale;
        localOrigin.y = (t.rotate.entry[0][1]*diff.x + t.rotate.entry[1][1]*diff.y + t.rotate.entry[2][1]*diff.z) / t.scale;
        localOrigin.z = (t.rotate.entry[0][2]*diff.x + t.rotate.entry[1][2]*diff.y + t.rotate.entry[2][2]*diff.z) / t.scale;

        // 2. Transform ray Direction Vector to Node Local Space
        RE::NiPoint3 localDir;
        localDir.x = (t.rotate.entry[0][0]*rayDirWorld.x + t.rotate.entry[1][0]*rayDirWorld.y + t.rotate.entry[2][0]*rayDirWorld.z) / t.scale;
        localDir.y = (t.rotate.entry[0][1]*rayDirWorld.x + t.rotate.entry[1][1]*rayDirWorld.y + t.rotate.entry[2][1]*rayDirWorld.z) / t.scale;
        localDir.z = (t.rotate.entry[0][2]*rayDirWorld.x + t.rotate.entry[1][2]*rayDirWorld.y + t.rotate.entry[2][2]*rayDirWorld.z) / t.scale;

        // 3. Perform AABB Check on Logical Dimensions
        AABB localAABB;
        float hScale = VRUISettings::get().hitboxScale;
        float depthScale = VRUISettings::get().hitTestDepth;
        float halfW = (_width * hScale) * 0.5f;
        float halfH = (_height * hScale) * 0.5f;
        
        // Use a robust depth tolerance (Y-axis) to provide a stable volume even with hand jitter
        localAABB.min = { -halfW, -1.0f * depthScale, -halfH };
        localAABB.max = {  halfW,  1.0f * depthScale,  halfH };

        return localAABB.intersectsRay(localOrigin, localDir, outDistance);
    }

    void VRUIWidget::update(float deltaTime)
    {
        // 1. Handle Animation
        if (_animDelayFrames > 0) {
            _animDelayFrames--;
            // Keep at zero scale while waiting
            if (_node) {
                _node->local.scale = 0.0f;
            }
        } 
        else if (_animProgress < 1.0f) {
            float animSpeed = 4.0f; // Seconds to full scale
            _animProgress += deltaTime * animSpeed;
            if (_animProgress > 1.0f) _animProgress = 1.0f;

            if (_node) {
                // Cubic Out easing: 1 - (1 - t)^3
                float t = _animProgress;
                float easedT = 1.0f - std::pow(1.0f - t, 3.0f);
                
                _node->local.scale = _baseScale * easedT;
                
                RE::NiUpdateData updateData;
                updateData.flags = RE::NiUpdateData::Flag::kDirty;
                _node->Update(updateData);
            }
        }

        // 2. Update children
        for (auto& child : _children) {
            child->update(deltaTime);
        }
    }

    void VRUIWidget::startScaleAnimation(int delayFrames)
    {
        _animDelayFrames = delayFrames;
        _animProgress = 0.0f;
        if (_node) {
            _node->local.scale = 0.0f;
            RE::NiUpdateData updateData;
            updateData.flags = RE::NiUpdateData::Flag::kDirty;
            _node->Update(updateData);
        }
    }

    void VRUIWidget::createNode()
    {
        _node.reset(RE::NiNode::Create(8));
        if (_node) {
            _node->name = _name;
        }
    }

    void VRUIWidget::initializeVisuals()
    {
        // Base implementation: no visuals to load.
        // Derived classes override this to load meshes.
    }

    // Recursively log the node tree for debugging
    static void logNodeTree(RE::NiAVObject* obj, int depth)
    {
        if (!obj) return;
        std::string indent(depth * 2, ' ');
        auto* node = obj->AsNode();
        if (node) {
            logger::info("{}[NiNode] '{}' children={} scale={:.2f} pos=({:.1f},{:.1f},{:.1f})",
                indent, node->name.c_str(), node->children.size(), node->local.scale,
                node->local.translate.x, node->local.translate.y, node->local.translate.z);
            for (auto& child : node->children) {
                if (child) {
                    logNodeTree(child.get(), depth + 1);
                }
            }
        } else {
            logger::info("{}[NiAVObject] '{}' (geometry/shape)", indent, obj->name.c_str());
        }
    }

    void VRUIWidget::logNodeHierarchy(const std::string& context) const
    {
        logger::info("ImmersiveUI: === Node Hierarchy [{}] ===", context);
        if (_node) {
            logNodeTree(_node.get(), 0);
        } else {
            logger::info("  (null node)");
        }
    }

    // =====================================================================
    // Load a NIF mesh using BSModelDB::Demand (game's native pipeline)
    // =====================================================================

    RE::NiPointer<RE::NiNode> VRUIWidget::loadModelFromNif(const std::string& nifPath)
    {
        // First, check the cache
        auto it = _nifCache.find(nifPath);
        if (it != _nifCache.end()) {
            // Found in cache, return a clone
            if (it->second) {
                auto* cloned = it->second->Clone();
                return RE::NiPointer<RE::NiNode>(cloned ? cloned->AsNode() : nullptr);
            }
        }

        RE::NiPointer<RE::NiNode> modelRoot;
        RE::BSModelDB::DBTraits::ArgsType args{};

        // Try multiple path variations to be as robust as possible
        std::vector<std::string> pathsToTry = { nifPath };
        
        // 1. Try with meshes\ prefix if not there
        if (!nifPath.starts_with("meshes\\") && !nifPath.starts_with("meshes/")) {
            pathsToTry.push_back("meshes\\" + nifPath);
        } else {
            // 2. Try without meshes\ prefix if it is there
            size_t pos = nifPath.find_first_of("\\/");
            if (pos != std::string::npos) {
                pathsToTry.push_back(nifPath.substr(pos + 1));
            }
        }

        RE::BSResource::ErrorCode result = RE::BSResource::ErrorCode::kNone;
        bool found = false;

        for (const auto& path : pathsToTry) {
            result = RE::BSModelDB::Demand(path.c_str(), modelRoot, args);
            if (result == RE::BSResource::ErrorCode::kNone && modelRoot) {
                found = true;
                logger::info("ImmersiveUI: BSModelDB::Demand success for path: '{}'", path);
                break;
            }
        }

        if (!found) {
            logger::warn("ImmersiveUI: BSModelDB::Demand failed for '{}' (tried {} variations, last error={})",
                nifPath, pathsToTry.size(), static_cast<int>(result));
            return nullptr;
        }

        // Store the original in cache
        _nifCache[nifPath] = modelRoot;
        logger::info("ImmersiveUI: Loaded NIF '{}' via BSModelDB::Demand and cached it", nifPath);

        // Clone the loaded model so we have our own instance
        auto* cloned = modelRoot->Clone();
        if (!cloned) {
            logger::warn("ImmersiveUI: Failed to clone model '{}'", nifPath);
            return nullptr;
        }

        auto* asNode = cloned->AsNode();
        return RE::NiPointer<RE::NiNode>(asNode);
    }

    RE::NiPointer<RE::NiNode> VRUIWidget::createQuadNode(
        const std::string& name, float width, float height,
        [[maybe_unused]] const RE::NiColorA& color)
    {
        // Create a container node for a quad panel.
        // For visual geometry, we load a built-in game mesh and rescale it.
        auto node = RE::NiPointer<RE::NiNode>(RE::NiNode::Create(2));
        if (!node) return nullptr;
        node->name = name;

        // Try loading a simple game marker mesh as a visual placeholder
        RE::NiPointer<RE::NiNode> meshNode;
        RE::BSModelDB::DBTraits::ArgsType args{};
        args.postProcess = false;  // Skip post-processing for simple use

        // Use a simple built-in mesh as the visual placeholder
    // We try immersiveUI\slot01.nif because we know it exists from the logs
    auto result = RE::BSModelDB::Demand("immersiveUI\\slot01.nif", meshNode, args);
    if (result != RE::BSResource::ErrorCode::kNone) {
        result = RE::BSModelDB::Demand("meshes\\immersiveUI\\slot01.nif", meshNode, args);
    }
    if (result != RE::BSResource::ErrorCode::kNone) {
        result = RE::BSModelDB::Demand("meshes\\markers\\movemarker01.nif", meshNode, args);
    }

        if (result == RE::BSResource::ErrorCode::kNone && meshNode) {
            // Clone and attach
            auto* cloned = meshNode->Clone();
            if (cloned && cloned->AsNode()) {
                auto* cloneNode = cloned->AsNode();
                cloneNode->local.scale = width * 0.1f;  // Scale to button size
                node->AttachChild(cloneNode);
                logger::info("ImmersiveUI: Created visual quad '{}' with game mesh", name);
            }
        } else {
            logger::warn("ImmersiveUI: Couldn't load visual mesh for '{}', using empty node", name);
        }

        return node;
    }
}
