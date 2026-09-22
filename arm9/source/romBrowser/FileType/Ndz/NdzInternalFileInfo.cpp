#include "common.h"
#include <string.h>
#include <nds/arm9/cache.h>
#include "fat/File.h"
#include "ndzFormat.h"
#include "NdzInternalFileInfo.h"

NdzInternalFileInfo::NdzInternalFileInfo(const FastFileRef& fastFileRef)
{
    memset(_gameCode, 0, sizeof(_gameCode));

    const auto file = std::make_unique<File>();
    file->Open(fastFileRef, FA_READ);

    u32 magic;
    if (file->Seek(NDZ_OFFSET_MAGIC) != FR_OK ||
        !file->ReadExact(&magic, sizeof(magic)) ||
        magic != NDZ_MAGIC)
    {
        return;
    }

    // the packer copies the .nds banner into a fixed slot, zero padded to sizeof(nds_banner_t)
    if (file->Seek(NDZ_OFFSET_BANNER) != FR_OK ||
        !file->ReadExact(&_banner, sizeof(_banner)))
    {
        return;
    }

    if (file->Seek(NDZ_OFFSET_GAMECODE) != FR_OK ||
        !file->ReadExact(_gameCode, 4))
    {
        return;
    }

    _hasBanner = true;
    DC_FlushRange(&_banner, sizeof(_banner));
}

const char16_t* NdzInternalFileInfo::GetGameTitle() const
{
    return _hasBanner
        ? _banner.title[NDS_BANNER_TITLE_LANGUAGE_ENGLISH]
        : nullptr;
}

std::unique_ptr<FileIcon> NdzInternalFileInfo::CreateGameIcon() const
{
    return _hasBanner
        ? std::make_unique<NdsFileIcon>(&_banner)
        : nullptr;
}
