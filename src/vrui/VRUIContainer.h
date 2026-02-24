#pragma once

#include "VRUIWidget.h"

namespace vrui
{
    /// Layout modes for arranging children
    enum class ContainerLayout : uint8_t
    {
        HorizontalCenter,  // Children arranged left-to-right, centered
        VerticalDown,       // Children arranged top-to-bottom
        VerticalUp,         // Children arranged bottom-to-top
        Grid,               // Children arranged in X/Y grid matrix
        Free                // No automatic layout, manual positioning
    };

    /// Container that arranges child widgets in a layout.
    /// Supports horizontal rows, vertical columns, and nesting.
    class VRUIContainer : public VRUIWidget
    {
    public:
        RE::NiPoint2 calculateLogicalDimensions() const override;

        /// @param name     Container identifier
        /// @param layout   Layout mode for children
        /// @param spacing  Space between children (in game units)
        /// @param scale    Scale of this container
        explicit VRUIContainer(const std::string& name,
                               ContainerLayout layout = ContainerLayout::VerticalDown,
                               float spacing = 0.3f,
                               float scale = 1.0f);

        void addElement(std::shared_ptr<VRUIWidget> element);
        void removeElement(const std::shared_ptr<VRUIWidget>& element);
        void clearElements();

        /// Recalculate positions of all children based on layout
        void recalculateLayout() override;

        ContainerLayout getLayout() const { return _layout; }
        void setLayout(ContainerLayout layout);
        void setSpacing(float spacing);

        void setPageSize(int size);
        void setPage(int page);
        int getPageSize() const { return _pageSize; }
        int getCurrentPage() const { return _currentPage; }
        int getTotalPages() const;
        void nextPage();
        void prevPage();

        void update(float deltaTime) override;

    private:
        bool _layoutDirty = true;
        ContainerLayout _layout;
        float _spacing;
        int _gridColumns = 3;  // Default for Grid layout
        int _pageSize = 0;     // 0 = no pagination
        int _currentPage = 0;
    };
}
