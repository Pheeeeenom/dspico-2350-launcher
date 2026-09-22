#pragma once
#include <memory>
#include "core/String.h"
#include "FileInfo.h"
#include "SdFolder.h"

/// @brief A delta .ndz rom hack joined to the base .ndz it was packed against.
struct NdzDeltaEntry
{
    const FileInfo* delta;
    const FileInfo* base;
    /// @brief "<hack name> <version>" from the delta's front matter.
    String<char, 96> label;
};

/// @brief Per-folder join of delta .ndz files to their base .ndz, built on the IO thread
///        right after the folder listing. A delta matches the base whose size and
///        header crc16 it was packed against.
class NdzDeltaIndex
{
public:
    /// @brief Reads every .ndz in \p sdFolder and flags deltas and their bases. IO thread only.
    static std::unique_ptr<NdzDeltaIndex> Build(SdFolder& sdFolder);

    u32 GetEntryCount() const { return _entryCount; }
    const NdzDeltaEntry& GetEntry(u32 index) const { return _entries[index]; }

private:
    std::unique_ptr<NdzDeltaEntry[]> _entries;
    u32 _entryCount = 0;
};
