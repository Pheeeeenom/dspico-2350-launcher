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
#define NDZ_OFFSET_HDRCRC          0x2418

#define NDZ_FLAG_DELTA             (1u << 4)

#define NDZ_OFFSET_DELTA_BASE_SIZE    0x2450
#define NDZ_OFFSET_DELTA_BASE_HDRCRC  0x2454
#define NDZ_OFFSET_DELTA_OPTIONS      0x2458
#define NDZ_DELTA_OPT_BASE_BINARIES   (1u << 0)   // retired: the loader still honours it
#define NDZ_DELTA_OPT_NTR_MODE        (1u << 1)
#define NDZ_OFFSET_DELTA_NAME         0x2460
#define NDZ_DELTA_NAME_LEN            64
#define NDZ_OFFSET_DELTA_VERSION      0x24A0
#define NDZ_DELTA_VERSION_LEN         32
// one read from NDZ_OFFSET_GAMECODE covers all the side fields
#define NDZ_SIDE_FIELDS_END        (NDZ_OFFSET_DELTA_VERSION + NDZ_DELTA_VERSION_LEN)
