#include "common.h"
#include <libtwl/dma/dmaNitro.h>
#include "gui/GraphicsContext.h"
#include "themes/material/MaterialColorScheme.h"
#include "themes/IFontRepository.h"
#include "gui/input/InputProvider.h"
#include "gui/VramContext.h"
#include "gui/IVramManager.h"
#include "gui/palette/GradientPalette.h"
#include "gui/OamBuilder.h"
#include "cheatSelector.h"
#include "HackSelectBottomSheetView.h"

#define TITLE_LABEL_X               20
#define TITLE_LABEL_Y               16

#define LIST_X                      16
#define LIST_Y                      40
#define LIST_WIDTH                  224
#define LIST_HEIGHT                 120

HackSelectBottomSheetView::HackSelectBottomSheetView(SharedPtr<HackSelectViewModel> viewModel,
    const MaterialColorScheme* materialColorScheme, const IFontRepository* fontRepository,
    FocusManager* focusManager)
    : _viewModel(std::move(viewModel))
    , _titleLabel(Label2DView::CreateShared(128, 16, 25, fontRepository->GetFont(FontType::Medium11)))
    , _recycler(RecyclerView::CreateShared(
        LIST_X, LIST_Y, LIST_WIDTH, LIST_HEIGHT, RecyclerView::Mode::VerticalList))
    , _materialColorScheme(materialColorScheme)
    , _fontRepository(fontRepository)
    , _focusManager(focusManager)
{
    _titleLabel->SetText(u"Versions");
    AddChildTail(_titleLabel.GetPointer());
    AddChildTail(_recycler.GetPointer());
}

void HackSelectBottomSheetView::InitVram(const VramContext& vramContext)
{
    BottomSheetView::InitVram(vramContext);

    const auto objVramManager = vramContext.GetObjVramManager();
    if (objVramManager)
    {
        _vramOffsets.selectorVramOffset = objVramManager->Alloc(cheatSelectorTilesLen);
        dma_ntrCopy32(3, cheatSelectorTiles,
            objVramManager->GetVramAddress(_vramOffsets.selectorVramOffset), cheatSelectorTilesLen);
    }
    _objVramManager = vramContext.GetObjVramManager();
}

void HackSelectBottomSheetView::Update()
{
    _titleLabel->SetPosition(TITLE_LABEL_X, _position.y + TITLE_LABEL_Y);
    _recycler->SetPosition(LIST_X, _position.y + LIST_Y);
    if (!_adapter && _objVramManager != nullptr)
    {
        _adapter = SharedPtr<HackSelectAdapter>::MakeShared(
            _viewModel, _materialColorScheme, _fontRepository, _vramOffsets);
        _recycler->SetAdapter(_adapter);
        _recycler->InitVram(VramContext(nullptr, _objVramManager, nullptr, nullptr));
        _recycler->Focus(*_focusManager);
    }
    BottomSheetView::Update();
    _viewModel->SetSelectedItem(_recycler->GetSelectedItem());
}

void HackSelectBottomSheetView::Draw(GraphicsContext& graphicsContext)
{
    graphicsContext.SetClipArea(GetBounds());
    u32 oldPrio = graphicsContext.SetPriority(1);
    {
        auto backColor = _materialColorScheme->GetColor(md::sys::color::surfaceContainerLow);

        if (_adapter)
        {
            graphicsContext.SetClipArea(_recycler->GetBounds());
            _recycler->Draw(graphicsContext);
            graphicsContext.SetClipArea(GetBounds());

            // masks scrolled-out rows above the list
            auto maskOam = graphicsContext.GetOamManager().AllocOams(4);
            u32 maskPaletteRow = graphicsContext.GetPaletteManager().AllocRow(
                GradientPalette(backColor, backColor),
                _position.y + LIST_Y - 24, _position.y + LIST_Y);
            for (int i = 0; i < 4; i++)
            {
                int x = LIST_X + (i < 3 ? i * 64 : 2 * 64 + 32);
                OamBuilder::OamWithSize<64, 32>(x, _position.y + LIST_Y - 24, _vramOffsets.selectorVramOffset >> 7)
                    .WithPalette16(maskPaletteRow)
                    .WithPriority(graphicsContext.GetPriority())
                    .Build(maskOam[i]);
            }
        }

        _titleLabel->SetBackgroundColor(backColor);
        _titleLabel->SetForegroundColor(_materialColorScheme->onSurface);
        _titleLabel->Draw(graphicsContext);
    }
    graphicsContext.SetPriority(oldPrio);
    graphicsContext.ResetClipArea();
}

void HackSelectBottomSheetView::Focus(FocusManager& focusManager)
{
    _recycler->Focus(focusManager);
}

bool HackSelectBottomSheetView::HandleInput(const InputProvider& inputProvider, FocusManager& focusManager)
{
    if (inputProvider.Triggered(InputKey::B))
    {
        _viewModel->Close();
        return true;
    }
    return false;
}

void HackSelectBottomSheetView::Close()
{
    _viewModel->Close();
}
