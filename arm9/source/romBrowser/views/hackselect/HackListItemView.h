#pragma once
#include "core/SharedPtr.h"
#include "gui/views/ViewContainer.h"
#include "gui/views/Label2DView.h"
#include "romBrowser/viewModels/HackSelectViewModel.h"

class MaterialColorScheme;
class IFontRepository;

/// @brief Version sheet row: version name plus a "base" or "hack" tag.
class HackListItemView : public ViewContainer
{
    SHARED_ONLY(HackListItemView)

public:
    struct VramOffsets
    {
        u32 selectorVramOffset = 0;
    };

    void Update() override;
    void Draw(GraphicsContext& graphicsContext) override;
    bool HandleInput(const InputProvider& inputProvider, FocusManager& focusManager) override;
    void HandlePenDown(const Point& touchPoint, FocusManager& focusManager) override;
    void HandlePenMove(const Point& touchPoint, FocusManager& focusManager) override;
    void HandlePenUp(const Point& lastTouchPoint, FocusManager& focusManager) override;

    Rectangle GetBounds() const override
    {
        return Rectangle(_position.x, _position.y, 224, 24);
    }

    void SetItem(const HackSelectViewModel::Item* item, int index);

private:
    SharedPtr<HackSelectViewModel> _viewModel;
    SharedPtr<Label2DView> _nameLabel;
    SharedPtr<Label2DView> _kindLabel;
    VramOffsets _vramOffsets;
    const MaterialColorScheme* _materialColorScheme;
    int _index = -1;
    bool _penDown = false;

    HackListItemView(SharedPtr<HackSelectViewModel> viewModel, const VramOffsets& vramOffsets,
        const MaterialColorScheme* materialColorScheme, const IFontRepository* fontRepository);
};
