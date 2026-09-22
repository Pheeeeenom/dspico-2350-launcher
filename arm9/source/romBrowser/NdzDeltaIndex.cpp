#include "common.h"
#include <string.h>
#include "core/mini-printf.h"
#include "fat/File.h"
#include "FileType/Ndz/NdzFileType.h"
#include "FileType/Ndz/ndzFormat.h"
#include "NdzDeltaIndex.h"

namespace
{
    struct NdzIdentity
    {
        bool isDelta;
        u32 originalSize;   // base: uncompressed .nds size
        u16 hdrCrc;         // base: crc16 of its .nds header (0 on packs that predate the field)
        u32 baseSize;       // delta: size of the base .nds it was packed against
        u16 baseHdrCrc;     // delta: header crc16 of that base
        char name[NDZ_DELTA_NAME_LEN];
        char version[NDZ_DELTA_VERSION_LEN];
    };

    // two small reads: the fixed header, then the side fields behind the banner slot
    bool ReadIdentity(const FastFileRef& fastFileRef, NdzIdentity& identity)
    {
        auto file = std::make_unique<File>();
        file->Open(fastFileRef, FA_READ);
        u8 head[16];
        if (file->Seek(NDZ_OFFSET_MAGIC) != FR_OK || !file->ReadExact(head, sizeof(head)))
            return false;
        u32 magic;
        memcpy(&magic, head + NDZ_OFFSET_MAGIC, 4);
        if (magic != NDZ_MAGIC)
            return false;
        u32 flags;
        memcpy(&identity.originalSize, head + NDZ_OFFSET_ORIGINAL_SIZE, 4);
        memcpy(&flags, head + NDZ_OFFSET_FLAGS, 4);
        identity.isDelta = (flags & NDZ_FLAG_DELTA) != 0;

        u8 side[NDZ_SIDE_FIELDS_END - NDZ_OFFSET_GAMECODE];
        if (file->Seek(NDZ_OFFSET_GAMECODE) != FR_OK || !file->ReadExact(side, sizeof(side)))
            return false;
        memcpy(&identity.hdrCrc, side + (NDZ_OFFSET_HDRCRC - NDZ_OFFSET_GAMECODE), 2);
        memcpy(&identity.baseSize, side + (NDZ_OFFSET_DELTA_BASE_SIZE - NDZ_OFFSET_GAMECODE), 4);
        memcpy(&identity.baseHdrCrc, side + (NDZ_OFFSET_DELTA_BASE_HDRCRC - NDZ_OFFSET_GAMECODE), 2);
        memcpy(identity.name, side + (NDZ_OFFSET_DELTA_NAME - NDZ_OFFSET_GAMECODE), NDZ_DELTA_NAME_LEN);
        memcpy(identity.version, side + (NDZ_OFFSET_DELTA_VERSION - NDZ_OFFSET_GAMECODE), NDZ_DELTA_VERSION_LEN);
        // the packer zero-pads both; a foreign file must not run off the end
        identity.name[NDZ_DELTA_NAME_LEN - 1] = 0;
        identity.version[NDZ_DELTA_VERSION_LEN - 1] = 0;
        return true;
    }

    struct NdzFile
    {
        FileInfo* file;
        NdzIdentity identity;
    };
}

// no std::vector: it pulls in the exception runtime, whose static constructor
// mallocs before the launcher's heap exists
std::unique_ptr<NdzDeltaIndex> NdzDeltaIndex::Build(SdFolder& sdFolder)
{
    auto index = std::make_unique<NdzDeltaIndex>();
    FileInfo* const* files = sdFolder.GetFiles();
    int fileCount = sdFolder.GetFileCount();
    u32 ndzCapacity = 0;
    for (int i = 0; i < fileCount; i++)
    {
        if (files[i]->GetFileType() == &NdzFileType::sInstance)
            ndzCapacity++;
    }
    if (ndzCapacity == 0)
        return index;

    auto ndzFiles = std::make_unique<NdzFile[]>(ndzCapacity);
    u32 ndzCount = 0;
    for (int i = 0; i < fileCount; i++)
    {
        FileInfo* file = files[i];
        if (file->GetFileType() != &NdzFileType::sInstance)
            continue;
        NdzFile& ndzFile = ndzFiles[ndzCount];
        ndzFile.file = file;
        if (!ReadIdentity(file->GetFastFileRef(), ndzFile.identity))
        {
            LOG_ERROR("Couldn't read the .ndz front matter of %s\n", file->GetFileName());
            continue;
        }
        ndzCount++;
    }

    u32 deltaCount = 0;
    for (u32 i = 0; i < ndzCount; i++)
    {
        if (ndzFiles[i].identity.isDelta)
            deltaCount++;
    }
    if (deltaCount == 0)
        return index;

    index->_entries = std::make_unique<NdzDeltaEntry[]>(deltaCount);
    for (u32 d = 0; d < ndzCount; d++)
    {
        const NdzFile& delta = ndzFiles[d];
        if (!delta.identity.isDelta)
            continue;
        // hidden even without its base here: an orphan can't be launched
        delta.file->SetNdzDelta(true);

        FileInfo* base = nullptr;
        for (u32 c = 0; c < ndzCount; c++)
        {
            const NdzFile& candidate = ndzFiles[c];
            const auto& id = candidate.identity;
            // a base packed before the crc field existed never attaches; repack it
            if (id.isDelta || id.hdrCrc == 0)
                continue;
            if (id.originalSize == delta.identity.baseSize && id.hdrCrc == delta.identity.baseHdrCrc)
            {
                base = candidate.file;
                break;
            }
        }
        if (!base)
        {
            LOG_ERROR("%s: base .ndz (size %u, header crc %04X) is not in this folder\n",
                delta.file->GetFileName(), (unsigned)delta.identity.baseSize,
                (unsigned)delta.identity.baseHdrCrc);
            continue;
        }

        auto& entry = index->_entries[index->_entryCount++];
        entry.delta = delta.file;
        entry.base = base;
        char label[97];
        const char* name = delta.identity.name[0] != 0 ? delta.identity.name : delta.file->GetFileName();
        if (delta.identity.version[0] != 0)
            mini_snprintf(label, sizeof(label), "%s %s", name, delta.identity.version);
        else
            mini_snprintf(label, sizeof(label), "%s", name);
        entry.label = label;
        base->SetNdzHackCount(base->GetNdzHackCount() + 1);
    }
    return index;
}
