#pragma once

// See https://github.com/Pheeeeenom/dspico-ir/blob/main/NDZ-SPEC.md

#define NDZ_MAGIC                  0x315A444Eu  // 'N','D','Z','1' little-endian
#define NDZ_FRONTMATTER_SIZE       0x4000       // 16 KB (cluster-aligned)
#define NDZ_BANNER_SLOT_SIZE       0x2400       // fits the largest (v103) banner

#define NDZ_OFFSET_MAGIC           0x0000
#define NDZ_OFFSET_FRONTMATTER_SZ  0x0004
#define NDZ_OFFSET_ORIGINAL_SIZE   0x0008
#define NDZ_OFFSET_FLAGS           0x000C
#define NDZ_OFFSET_BANNER          0x0010
#define NDZ_OFFSET_GAMECODE        (NDZ_OFFSET_BANNER + NDZ_BANNER_SLOT_SIZE)
