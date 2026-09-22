#pragma once
#include <memory>
#include "core/SharedPtr.h"
#include "romBrowser/views/BottomSheetView.h"
#include "gui/views/Label2DView.h"
#include "gui/views/RecyclerView.h"
#include "romBrowser/viewModels/HackSelectViewModel.h"
#include "HackSelectAdapter.h"

class MaterialColorScheme;
class IFontRepository;
class IVramManager;

/// @brief Bottom sheet listing a base .ndz and its rom hacks. A launches the highlighted
///        version and B goes back. X flips a hack between NTR and TWL mode.
class HackSelectBottomSheetView : public BottomSheetView
{
    SHARED_ONLY(HackSelectBottomSheetView)

public:
    void InitVram(const VramContext& vramContext) override;
    void Update() override;
    void Draw(GraphicsContext& graphicsContext) override;
    bool HandleInput(const InputProvider& inputProvider, FocusManager& focusManager) override;

    void Focus(FocusManager& focusManager) override;

protected:
    void Close() override;

private:
    SharedPtr<HackSelectViewModel> _viewModel;
    SharedPtr<Label2DView> _titleLabel;
    SharedPtr<RecyclerView> _recycler;
    SharedPtr<HackSelectAdapter> _adapter;
    const MaterialColorScheme* _materialColorScheme;
    const IFontRepository* _fontRepository;
    IVramManager* _objVramManager = nullptr;
    FocusManager* _focusManager;
    HackListItemView::VramOffsets _vramOffsets;

    HackSelectBottomSheetView(SharedPtr<HackSelectViewModel> viewModel,
        const MaterialColorScheme* materialColorScheme, const IFontRepository* fontRepository,
        FocusManager* focusManager);
};
