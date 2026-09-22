#pragma once
#include "gui/views/RecyclerAdapter.h"
#include "romBrowser/viewModels/HackSelectViewModel.h"
#include "HackListItemView.h"

/// @brief Recycler adapter for the version sheet.
class HackSelectAdapter : public RecyclerAdapter
{
public:
    HackSelectAdapter(SharedPtr<HackSelectViewModel> viewModel,
        const MaterialColorScheme* materialColorScheme, const IFontRepository* fontRepository,
        const HackListItemView::VramOffsets& vramOffsets)
        : _viewModel(std::move(viewModel)), _materialColorScheme(materialColorScheme)
        , _fontRepository(fontRepository), _vramOffsets(vramOffsets) { }

    u32 GetItemCount() const override
    {
        return _viewModel->GetItemCount();
    }

    void GetViewSize(int& width, int& height) const override
    {
        width = 224;
        height = 24;
    }

    SharedPtr<View> CreateView() const override
    {
        return HackListItemView::CreateShared(_viewModel, _vramOffsets, _materialColorScheme, _fontRepository);
    }

    void BindView(SharedPtr<View> view, int index) const override
    {
        auto listItemView = static_cast<HackListItemView*>(view.GetPointer());
        listItemView->SetItem(&_viewModel->GetItem(index), index);
    }

    void ReleaseView(SharedPtr<View> view, int index) const override
    {
        // Nothing to do
    }

private:
    SharedPtr<HackSelectViewModel> _viewModel;
    const MaterialColorScheme* _materialColorScheme;
    const IFontRepository* _fontRepository;
    HackListItemView::VramOffsets _vramOffsets;
};
