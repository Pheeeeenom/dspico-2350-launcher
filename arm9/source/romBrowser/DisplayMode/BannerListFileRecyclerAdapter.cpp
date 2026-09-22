#include "common.h"
#include "core/mini-printf.h"
#include "../FileInfoManager.h"
#include "core/task/TaskQueue.h"
#include "../views/BannerListItemView.h"
#include "../Theme/IRomBrowserViewFactory.h"
#include "romBrowser/viewModels/RomBrowserItemViewModel.h"
#include "BannerListFileRecyclerAdapter.h"

// "<first title line> (2 hacks)", the rest of the title kept
static void TitleWithHackCount(const char16_t* title, u32 hackCount, char16_t* out, u32 outLength)
{
    u32 n = 0;
    u32 i = 0;
    while (title[i] != 0 && title[i] != u'\n' && n + 1 < outLength)
        out[n++] = title[i++];
    char hint[16];
    mini_snprintf(hint, sizeof(hint), " (%u hack%s)", (unsigned)hackCount, hackCount == 1 ? "" : "s");
    for (const char* p = hint; *p != 0 && n + 1 < outLength; p++)
        out[n++] = (char16_t)*p;
    while (title[i] != 0 && n + 1 < outLength)
        out[n++] = title[i++];
    out[n] = 0;
}

void BannerListFileRecyclerAdapter::GetViewSize(int& width, int& height) const
{
    width = 203;
    height = 44;
}

SharedPtr<View> BannerListFileRecyclerAdapter::CreateView() const
{
    return _romBrowserViewFactory->CreateBannerListItemView(
        std::make_unique<RomBrowserItemViewModel>(_romBrowserController), _vblankTextureLoader);
}

void BannerListFileRecyclerAdapter::BindView(SharedPtr<View> view, int index) const
{
    auto listItemView = static_cast<BannerListItemView*>(view.GetPointer());
    listItemView->SetGraphics(_bannerListItemViewGraphics);
    FileRecyclerAdapter::BindView(view, index);
}

TaskResult<void> BannerListFileRecyclerAdapter::BindView(SharedPtr<View> view, int index,
    const InternalFileInfo* internalFileInfo, const vu8& cancelRequested) const
{
    auto listItemView = static_cast<BannerListItemView*>(view.GetPointer());
    listItemView->GetViewModel().SetIndex(index);
    const auto& fileInfo = _fileInfoManager->GetItem(index);
    bool fileNameAsTitle = true;
    if (internalFileInfo)
    {
        const char16_t* gameTitle = internalFileInfo->GetGameTitle();
        if (gameTitle && gameTitle[0] != 0)
        {
            if (fileInfo.GetNdzHackCount() > 0)
            {
                char16_t titled[144];
                TitleWithHackCount(gameTitle, fileInfo.GetNdzHackCount(), titled,
                    sizeof(titled) / sizeof(titled[0]));
                listItemView->SetGameTitle(titled);
            }
            else
            {
                listItemView->SetGameTitle(gameTitle);
            }
            fileNameAsTitle = false;
        }
    }
    listItemView->SetFileName(fileInfo.GetFileName(), fileNameAsTitle);
    auto icon = internalFileInfo ? internalFileInfo->CreateGameIcon() : nullptr;
    if (!icon)
    {
        icon = fileInfo.GetFileType()->CreateFileIcon("", _themeFileIconFactory);
    }
    if (icon != nullptr)
    {
        if (cancelRequested)
        {
            icon.reset();
            _fileInfoManager->ReleaseFileInfo(index);
            return TaskResult<void>::Canceled();
        }
        icon->SetAnimFrame(_iconFrameCounter);
        listItemView->SetIcon(std::move(icon));
        listItemView->UploadIconGraphics();
        if (cancelRequested)
        {
            listItemView->SetIcon(nullptr);
            _fileInfoManager->ReleaseFileInfo(index);
            return TaskResult<void>::Canceled();
        }
    }
    return TaskResult<void>::Completed();
}

void BannerListFileRecyclerAdapter::SetQueueTask(const SharedPtr<View>& view, QueueTask<void> queueTask) const
{
    auto listItemView = static_cast<BannerListItemView*>(view.GetPointer());
    listItemView->GetViewModel().SetQueueTask(std::move(queueTask));
}

void BannerListFileRecyclerAdapter::ReleaseView(SharedPtr<View> view, int index) const
{
    LOG_DEBUG("Releasing %d\n", index);
    auto listItemView = static_cast<BannerListItemView*>(view.GetPointer());
    listItemView->SetIcon(nullptr);
    listItemView->SetGameTitle(u"");
    listItemView->GetViewModel().SetIndex(-1);
    listItemView->GetViewModel().CancelQueueTask();
    _fileInfoManager->ReleaseFileInfo(index);
}

void BannerListFileRecyclerAdapter::InitVram(const VramContext& vramContext)
{
    _bannerListItemViewGraphics = _romBrowserViewFactory->UploadBannerListItemViewGraphics(vramContext);
}
