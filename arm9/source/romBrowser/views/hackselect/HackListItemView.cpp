#include "common.h"
#include "themes/material/MaterialColorScheme.h"
#include "themes/IFontRepository.h"
#include "gui/palette/GradientPalette.h"
#include "gui/GraphicsContext.h"
#include "gui/OamBuilder.h"
#include "gui/input/InputProvider.h"
#include "HackListItemView.h"

#define NAME_LABEL_X       8
#define NAME_LABEL_Y       5
#define NAME_LABEL_WIDTH   164

#define KIND_LABEL_X       (NAME_LABEL_X + NAME_LABEL_WIDTH + 4)
#define KIND_LABEL_Y       7
#define KIND_LABEL_WIDTH   44

HackListItemView::HackListItemView(SharedPtr<HackSelectViewModel> viewModel, const VramOffsets& vramOffsets,
    const MaterialColorScheme* materialColorScheme, const IFontRepository* fontRepository)
    : _viewModel(std::move(viewModel))
    , _nameLabel(Label2DView::CreateShared(NAME_LABEL_WIDTH, 16, 96, fontRepository->GetFont(FontType::Regular10)))
    , _kindLabel(Label2DView::CreateShared(KIND_LABEL_WIDTH, 16, 8, fontRepository->GetFont(FontType::Medium7_5)))
    , _vramOffsets(vramOffsets)
    , _materialColorScheme(materialColorScheme)
{
    _nameLabel->SetEllipsisStyle(LabelView::EllipsisStyle::Ellipsis);
    _kindLabel->SetHorizontalAlignment(Alignment::End);
    AddChildTail(_nameLabel.GetPointer());
    AddChildTail(_kindLabel.GetPointer());
}

void HackListItemView::SetItem(const HackSelectViewModel::Item* item, int index)
{
    _index = index;
    _nameLabel->SetText(item->name.GetString());
    _kindLabel->SetText(item->kind);
}

void HackListItemView::Update()
{
    _nameLabel->SetPosition(_position.x + NAME_LABEL_X, _position.y + NAME_LABEL_Y);
    _kindLabel->SetPosition(_position.x + KIND_LABEL_X, _position.y + KIND_LABEL_Y);
    _nameLabel->SetEllipsisStyle(IsFocused()
        ? LabelView::EllipsisStyle::Marquee
        : LabelView::EllipsisStyle::Ellipsis);
    ViewContainer::Update();
}

void HackListItemView::Draw(GraphicsContext& graphicsContext)
{
    if (!graphicsContext.IsVisible(GetBounds()))
        return;

    auto backColor = _materialColorScheme->GetColor(md::sys::color::surfaceContainerLow);
    if (IsFocused())
    {
        auto selectorFullColor = _materialColorScheme->GetColor(md::sys::color::onSurface);
        backColor = RgbMixer::Lerp(backColor, selectorFullColor, 10, 100);
    }

    _nameLabel->SetBackgroundColor(backColor);
    _nameLabel->SetForegroundColor(_materialColorScheme->onSurface);
    _kindLabel->SetBackgroundColor(backColor);
    _kindLabel->SetForegroundColor(_materialColorScheme->onSurfaceVariant);

    if (IsFocused())
    {
        u32 selectorPaletteRow = graphicsContext.GetPaletteManager().AllocRow(
            GradientPalette(backColor, backColor),
            _position.y, _position.y + 24);
        auto selectorOam = graphicsContext.GetOamManager().AllocOams(4);
        for (int i = 0; i < 4; i++)
        {
            int x = _position.x + (i < 3 ? i * 64 : 2 * 64 + 32);
            OamBuilder::OamWithSize<64, 32>(x, _position.y, _vramOffsets.selectorVramOffset >> 7)
                .WithPalette16(selectorPaletteRow)
                .WithPriority(graphicsContext.GetPriority())
                .Build(selectorOam[i]);
        }
    }

    ViewContainer::Draw(graphicsContext);
}

bool HackListItemView::HandleInput(const InputProvider& inputProvider, FocusManager& focusManager)
{
    if (inputProvider.Triggered(InputKey::A))
    {
        _viewModel->ActivateItem(_index);
        return true;
    }
    return ViewContainer::HandleInput(inputProvider, focusManager);
}

void HackListItemView::HandlePenDown(const Point& touchPoint, FocusManager& focusManager)
{
    if (GetBounds().Contains(touchPoint))
        _penDown = true;
}

void HackListItemView::HandlePenMove(const Point& touchPoint, FocusManager& focusManager)
{
    if (!GetBounds().Contains(touchPoint))
        _penDown = false;
}

void HackListItemView::HandlePenUp(const Point& lastTouchPoint, FocusManager& focusManager)
{
    if (_penDown && GetBounds().Contains(lastTouchPoint))
    {
        if (focusManager.GetCurrentFocus().GetPointer() == this)
            _viewModel->ActivateItem(_index);
        else
            focusManager.Focus(SharedFromThis());
    }
    _penDown = false;
}
