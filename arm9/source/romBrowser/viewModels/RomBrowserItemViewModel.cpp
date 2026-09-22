#include "common.h"
#include "romBrowser/IRomBrowserController.h"
#include "RomBrowserViewModel.h"
#include "romBrowser/FileType/Nds/NdsFileType.h"
#include "RomBrowserItemViewModel.h"

void RomBrowserItemViewModel::Activate()
{
    if (_index >= 0)
    {
        const auto& item = _romBrowserController->GetRomBrowserViewModel()->GetFileInfoManager().GetItem(_index);
        if (item.GetFileType()->GetClassification() == FileTypeClassification::Folder)
        {
            _romBrowserController->NavigateToPath(item.GetFileName());
        }
        else if (item.GetNdzHackCount() > 0)
        {
            // base .ndz with rom hacks attached: pick which version to play
            _romBrowserController->ShowHackSelect(item);
        }
        else
        {
            _romBrowserController->LaunchFile(item);
        }
    }
}

void RomBrowserItemViewModel::ShowGameInfo()
{
    if (_index >= 0)
    {
        const auto& item = _romBrowserController->GetRomBrowserViewModel()->GetFileInfoManager().GetItem(_index);
        if (item.GetFileType() == &NdsFileType::sInstance)
        {
            _romBrowserController->ShowGameInfo(item);
        }
    }
}
