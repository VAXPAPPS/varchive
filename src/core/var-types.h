/*
 * VArchive - Professional Archive Manager for VAXP-OS
 * Copyright (C) 2026 VAXP Organization
 *
 * var-types.h — Common types, enums, and macros
 */

#pragma once

#include <glib.h>

G_BEGIN_DECLS

/* ═══════════════════════════════════════════════════
   Application Constants
   ═══════════════════════════════════════════════════ */

#define VAR_APP_ID      "org.vaxp.archive"
#define VAR_APP_NAME    "VArchive"
#define VAR_APP_VERSION "1.0.0"

/* ═══════════════════════════════════════════════════
   Archive Format Types
   ═══════════════════════════════════════════════════ */

typedef enum {
    VAR_FORMAT_UNKNOWN = 0,

    /* Multi-file archive formats */
    VAR_FORMAT_ZIP,
    VAR_FORMAT_TAR,
    VAR_FORMAT_TAR_GZ,
    VAR_FORMAT_TAR_BZ2,
    VAR_FORMAT_TAR_XZ,
    VAR_FORMAT_TAR_ZST,
    VAR_FORMAT_TAR_LZ,
    VAR_FORMAT_TAR_LZOP,
    VAR_FORMAT_TAR_LRZ,
    VAR_FORMAT_TAR_BR,
    VAR_FORMAT_TAR_BZ,
    VAR_FORMAT_TAR_BZ3,
    VAR_FORMAT_TAR_Z,
    VAR_FORMAT_TAR_7Z,
    VAR_FORMAT_SEVENZ,
    VAR_FORMAT_RAR,
    VAR_FORMAT_ARJ,
    VAR_FORMAT_ACE,
    VAR_FORMAT_ALZ,
    VAR_FORMAT_AR,
    VAR_FORMAT_CAB,
    VAR_FORMAT_CPIO,
    VAR_FORMAT_LHA,
    VAR_FORMAT_ZOO,
    VAR_FORMAT_JAR,
    VAR_FORMAT_WAR,
    VAR_FORMAT_EAR,
    VAR_FORMAT_CBZ,
    VAR_FORMAT_CBR,

    /* Read-only package/image formats */
    VAR_FORMAT_DEB,
    VAR_FORMAT_RPM,
    VAR_FORMAT_ISO,
    VAR_FORMAT_DMG,
    VAR_FORMAT_SNAP,
    VAR_FORMAT_SQUASHFS,
    VAR_FORMAT_STUFFIT,

    /* Single-file compression */
    VAR_FORMAT_GZ,
    VAR_FORMAT_BZ2,
    VAR_FORMAT_XZ,
    VAR_FORMAT_ZST,
    VAR_FORMAT_LZ,
    VAR_FORMAT_LZOP,
    VAR_FORMAT_LRZ,
    VAR_FORMAT_BR,
    VAR_FORMAT_BZ,
    VAR_FORMAT_BZ3,
    VAR_FORMAT_Z,
    VAR_FORMAT_RZ,

    VAR_FORMAT_COUNT
} VarFormatType;

/* ═══════════════════════════════════════════════════
   Archive Entry Types
   ═══════════════════════════════════════════════════ */

typedef enum {
    VAR_ENTRY_FILE,
    VAR_ENTRY_DIRECTORY,
    VAR_ENTRY_SYMLINK,
    VAR_ENTRY_HARDLINK,
    VAR_ENTRY_SPECIAL,
} VarEntryType;

/* ═══════════════════════════════════════════════════
   Compression Levels
   ═══════════════════════════════════════════════════ */

typedef enum {
    VAR_COMPRESS_STORE = 0,
    VAR_COMPRESS_FASTEST = 1,
    VAR_COMPRESS_FAST = 3,
    VAR_COMPRESS_NORMAL = 5,
    VAR_COMPRESS_GOOD = 7,
    VAR_COMPRESS_BEST = 9,
} VarCompressionLevel;

/* ═══════════════════════════════════════════════════
   Operation Types
   ═══════════════════════════════════════════════════ */

typedef enum {
    VAR_OP_NONE,
    VAR_OP_LIST,
    VAR_OP_EXTRACT,
    VAR_OP_CREATE,
    VAR_OP_ADD,
    VAR_OP_DELETE,
    VAR_OP_TEST,
} VarOperationType;

/* ═══════════════════════════════════════════════════
   Overwrite Policy
   ═══════════════════════════════════════════════════ */

typedef enum {
    VAR_OVERWRITE_ASK,
    VAR_OVERWRITE_ALWAYS,
    VAR_OVERWRITE_SKIP,
    VAR_OVERWRITE_RENAME,
} VarOverwritePolicy;

/* ═══════════════════════════════════════════════════
   Progress Callback
   ═══════════════════════════════════════════════════ */

typedef void (*VarProgressCallback)(
    double       fraction,       /* 0.0 – 1.0 */
    const char  *current_file,   /* file being processed */
    gint64       bytes_done,
    gint64       bytes_total,
    gpointer     user_data
);

/* ═══════════════════════════════════════════════════
   Backend Capability Flags
   ═══════════════════════════════════════════════════ */

typedef enum {
    VAR_CAP_READ       = 1 << 0,
    VAR_CAP_WRITE      = 1 << 1,
    VAR_CAP_LIST       = 1 << 2,
    VAR_CAP_ADD        = 1 << 3,
    VAR_CAP_DELETE     = 1 << 4,
    VAR_CAP_TEST       = 1 << 5,
    VAR_CAP_PASSWORD   = 1 << 6,
    VAR_CAP_MULTIPART  = 1 << 7,
} VarCapabilityFlags;

G_END_DECLS
