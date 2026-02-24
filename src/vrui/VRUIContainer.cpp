#include "VRUIContainer.h"
#include <cmath>
#include <algorithm>
#include "VRUISettings.h"

#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif
namespace vrui
{
    RE::NiPoint2 VRUIContainer::calculateLogicalDimensions() const
    {
        const auto& children = getChildren();
        if (children.empty()) return { 0.0f, 0.0f };

        float minX = 0.0f, maxX = 0.0f, minZ = 0.0f, maxZ = 0.0f;
        bool first = true;

        for (const auto& child : children) {
            if (!child->isVisible()) continue;

            RE::NiPoint3 pos = child->getLocalPosition();
            RE::NiPoint2 size = child->calculateLogicalDimensions();

            float left = pos.x - size.x * 0.5f;
            float right = pos.x + size.x * 0.5f;
            float top = pos.z + size.y * 0.5f;
            float bottom = pos.z - size.y * 0.5f;

            if (first) {
                minX = left; maxX = right;
                minZ = bottom; maxZ = top;
                first = false;
            } else {
                minX = std::min(minX, left);
                maxX = std::max(maxX, right);
                minZ = std::min(minZ, bottom);
                maxZ = std::max(maxZ, top);
            }
        }

        if (first) return { 0.0f, 0.0f };
        return { maxX - minX, maxZ - minZ };
    }

    VRUIContainer::VRUIContainer(const std::string& name,
                                 ContainerLayout layout,
                                 float spacing,
                                 float scale)
        : VRUIWidget(name, 0, 0)  // Container has no intrinsic size
        , _layout(layout)
        , _spacing(spacing)
        , _pageSize(0)
        , _currentPage(0)
    {
        setLocalScale(scale);
    }

    void VRUIContainer::addElement(std::shared_ptr<VRUIWidget> element)
    {
        addChild(std::move(element));
        recalculateLayout();
    }

    void VRUIContainer::removeElement(const std::shared_ptr<VRUIWidget>& element)
    {
        removeChild(element);
        recalculateLayout();
    }

    void VRUIContainer::clearElements()
    {
        auto children = getChildren();  // copy
        for (auto& child : children) {
            removeChild(child);
        }
    }

    void VRUIContainer::setLayout(ContainerLayout layout)
    {
        _layout = layout;
        recalculateLayout();
    }

    void VRUIContainer::setSpacing(float spacing)
    {
        _spacing = spacing;
        recalculateLayout();
    }

    void VRUIContainer::setPageSize(int size)
    {
        _pageSize = std::max(0, size);
        _currentPage = 0;
        recalculateLayout();
    }

    void VRUIContainer::setPage(int page)
    {
        int total = getTotalPages();
        if (total > 0) {
            _currentPage = std::clamp(page, 0, total - 1);
        } else {
            _currentPage = 0;
        }
        recalculateLayout();

        // Trigger cascade entrance animation on newly visible children
        int visibleIdx = 0;
        for (auto& child : _children) {
            if (child && child->isVisible()) {
                child->startScaleAnimation(visibleIdx * 2);
                visibleIdx++;
            }
        }
    }

    int VRUIContainer::getTotalPages() const
    {
        if (_pageSize <= 0) return 1;
        int numVisible = static_cast<int>(_children.size());
        if (numVisible == 0) return 1;
        return static_cast<int>(std::ceil((float)numVisible / _pageSize));
    }

    void VRUIContainer::nextPage()
    {
        int total = getTotalPages();
        if (_currentPage + 1 < total) {
            setPage(_currentPage + 1);
        } else {
            setPage(0); // Wrap around?
        }
    }

    void VRUIContainer::prevPage()
    {
        if (_currentPage > 0) {
            setPage(_currentPage - 1);
        } else {
            setPage(getTotalPages() - 1); // Wrap around?
        }
    }

    void VRUIContainer::recalculateLayout()
    {
        _layoutDirty = false;

        const auto& children = getChildren();

        // Recursively trigger layout update for all children containers first
        for (auto& child : children) {
            child->recalculateLayout();
        }

        if (children.empty()) return;

        switch (_layout) {
        case ContainerLayout::HorizontalCenter: {
            // Calculate total width of visible children
            float totalWidth = 0.0f;
            int visibleCount = 0;
            for (const auto& child : children) {
                if (child->isVisible()) {
                    totalWidth += child->calculateLogicalDimensions().x;
                    visibleCount++;
                }
            }
            if (visibleCount > 1) totalWidth += _spacing * (visibleCount - 1);

            // Position centered
            float currentX = -totalWidth * 0.5f;
            for (const auto& child : children) {
                if (!child->isVisible()) continue;
                float childW = child->calculateLogicalDimensions().x;
                child->setLocalPosition(RE::NiPoint3{ currentX + childW * 0.5f, 0.0f, 0.0f });
                currentX += childW + _spacing;
            }
            break;
        }

        case ContainerLayout::VerticalDown: {
            float currentZ = 0.0f; // Start from top
            for (const auto& child : children) {
                if (!child->isVisible()) continue;
                float childH = child->calculateLogicalDimensions().y;
                // Move currentZ half-way into this widget, position it, then move the rest out
                child->setLocalPosition(RE::NiPoint3{ 0.0f, 0.0f, currentZ - childH * 0.5f });
                currentZ -= (childH + _spacing);
            }
            break;
        }

        case ContainerLayout::VerticalUp: {
            float currentZ = 0.0f; // Start from bottom
            for (const auto& child : children) {
                if (!child->isVisible()) continue;
                float childH = child->calculateLogicalDimensions().y;
                child->setLocalPosition(RE::NiPoint3{ 0.0f, 0.0f, currentZ + childH * 0.5f });
                currentZ += (childH + _spacing);
            }
            break;
        }

        case ContainerLayout::Grid: {
            // Filter only visible children for the grid (not including those hidden by pagination)
            // But actually we decide WHO is visible based on pagination HERE.
            std::vector<VRUIWidget*> eligibleChildren;
            for (const auto& child : _children) {
                // We only consider children that haven't been forced hidden by other logic
                eligibleChildren.push_back(child.get());
            }

            int numEligible = static_cast<int>(eligibleChildren.size());
            if (numEligible == 0) break;

            // Apply Pagination
            int startIndex = 0;
            int endIndex = numEligible;
            
            if (_pageSize > 0) {
                startIndex = _currentPage * _pageSize;
                endIndex = std::min(startIndex + _pageSize, numEligible);
                
                // If current page is out of bounds, reset to 0
                if (startIndex >= numEligible && _currentPage > 0) {
                    _currentPage = 0;
                    recalculateLayout(); 
                    return;
                }
            }

            // Hide everything first
            for (auto* child : eligibleChildren) {
                child->setVisible(false);
            }

            std::vector<VRUIWidget*> pageChildren;
            for (int i = startIndex; i < endIndex; ++i) {
                eligibleChildren[i]->setVisible(true);
                pageChildren.push_back(eligibleChildren[i]);
            }

            int numInPage = static_cast<int>(pageChildren.size());
            if (numInPage == 0) break;

            int rows = std::ceil((float)numInPage / _gridColumns);
            int cols = std::min(numInPage, _gridColumns);

            float visualScale = VRUISettings::get().buttonMeshScale;
            float cellW = pageChildren[0]->getWidth() * visualScale;
            float cellH = pageChildren[0]->getHeight() * visualScale;

            float totalWidth = (cols * cellW) + ((cols - 1) * _spacing);
            float totalHeight = (rows * cellH) + ((rows - 1) * _spacing);

            float startX = -totalWidth * 0.5f + (cellW * 0.5f);
            float startZ = totalHeight * 0.5f - (cellH * 0.5f);

            for (int i = 0; i < numInPage; ++i) {
                int r = i / _gridColumns;
                int c = i % _gridColumns;

                if (VRUISettings::get().invertGridX) {
                    c = (cols - 1) - c;
                }

                float cx = startX + c * (cellW + _spacing);
                float cz = startZ - r * (cellH + _spacing);
                
                pageChildren[i]->setLocalPosition(RE::NiPoint3{ cx, 0.0f, cz });
            }
            break;
        }

        case ContainerLayout::Free:
            // No automatic layout
            break;
        }

        // --- FINAL DIMENSION UPDATE ---
        // Now that children are positioned, update our own reported width/height 
        // based on the logical bounds of all visible children.
        RE::NiPoint2 dims = calculateLogicalDimensions();
        _width = dims.x;
        _height = dims.y;
    }

    void VRUIContainer::update(float deltaTime)
    {
        VRUIWidget::update(deltaTime);
    }
}
