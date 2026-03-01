/*
 * VArchive - Professional Archive Manager for VAXP-OS
 * Copyright (C) 2026 VAXP Organization
 *
 * var-utils.c — Utility functions implementation
 */

#include "var-utils.h"
#include <string.h>
#include <strings.h>
#include <time.h>

/* ═══════════════════════════════════════════════════
   Format Size
   ═══════════════════════════════════════════════════ */

char *
var_utils_format_size (gint64 bytes)
{
    if (bytes < 0) return g_strdup ("—");

    const char *units[] = { "B", "KB", "MB", "GB", "TB" };
    int idx = 0;
    double size = (double)bytes;

    while (size >= 1024.0 && idx < 4) {
        size /= 1024.0;
        idx++;
    }

    if (idx == 0)
        return g_strdup_printf ("%" G_GINT64_FORMAT " B", bytes);
    else
        return g_strdup_printf ("%.1f %s", size, units[idx]);
}

/* ═══════════════════════════════════════════════════
   Format Date
   ═══════════════════════════════════════════════════ */

char *
var_utils_format_date (gint64 timestamp)
{
    if (timestamp <= 0)
        return g_strdup ("—");

    time_t t = (time_t)timestamp;
    struct tm tm_buf;
    localtime_r (&t, &tm_buf);

    char buf[64];
    strftime (buf, sizeof(buf), "%Y-%m-%d %H:%M", &tm_buf);
    return g_strdup (buf);
}

/* ═══════════════════════════════════════════════════
   File Icon
   ═══════════════════════════════════════════════════ */

const char *
var_utils_get_file_icon_name (const char *filename)
{
    if (!filename) return "text-x-generic-symbolic";

    const char *ext = strrchr (filename, '.');
    if (!ext) return "text-x-generic-symbolic";

    ext++; /* skip the dot */

    /* Images */
    if (strcasecmp(ext, "png") == 0 || strcasecmp(ext, "jpg") == 0 ||
        strcasecmp(ext, "jpeg") == 0 || strcasecmp(ext, "gif") == 0 ||
        strcasecmp(ext, "svg") == 0 || strcasecmp(ext, "webp") == 0)
        return "image-x-generic-symbolic";

    /* Audio */
    if (strcasecmp(ext, "mp3") == 0 || strcasecmp(ext, "wav") == 0 ||
        strcasecmp(ext, "ogg") == 0 || strcasecmp(ext, "flac") == 0)
        return "audio-x-generic-symbolic";

    /* Video */
    if (strcasecmp(ext, "mp4") == 0 || strcasecmp(ext, "mkv") == 0 ||
        strcasecmp(ext, "avi") == 0 || strcasecmp(ext, "webm") == 0)
        return "video-x-generic-symbolic";

    /* Code */
    if (strcasecmp(ext, "c") == 0 || strcasecmp(ext, "h") == 0 ||
        strcasecmp(ext, "py") == 0 || strcasecmp(ext, "js") == 0 ||
        strcasecmp(ext, "java") == 0 || strcasecmp(ext, "rs") == 0)
        return "text-x-script-symbolic";

    /* Documents */
    if (strcasecmp(ext, "pdf") == 0)
        return "x-office-document-symbolic";

    return "text-x-generic-symbolic";
}

/* ═══════════════════════════════════════════════════
   Extension ↔ Format Mapping
   ═══════════════════════════════════════════════════ */

typedef struct {
    const char   *extension;
    VarFormatType format;
} ExtMap;

static const ExtMap ext_map[] = {
    /* Multi-extension tar variants (must be checked first) */
    { ".tar.gz",  VAR_FORMAT_TAR_GZ  },
    { ".tgz",     VAR_FORMAT_TAR_GZ  },
    { ".tar.bz2", VAR_FORMAT_TAR_BZ2 },
    { ".tbz2",    VAR_FORMAT_TAR_BZ2 },
    { ".tar.bz",  VAR_FORMAT_TAR_BZ  },
    { ".tbz",     VAR_FORMAT_TAR_BZ  },
    { ".tar.xz",  VAR_FORMAT_TAR_XZ  },
    { ".tar.zst", VAR_FORMAT_TAR_ZST },
    { ".tzst",    VAR_FORMAT_TAR_ZST },
    { ".tar.lz",  VAR_FORMAT_TAR_LZ  },
    { ".tlz",     VAR_FORMAT_TAR_LZ  },
    { ".tar.lzo", VAR_FORMAT_TAR_LZOP },
    { ".tzo",     VAR_FORMAT_TAR_LZOP },
    { ".tar.lrz", VAR_FORMAT_TAR_LRZ },
    { ".tlrz",    VAR_FORMAT_TAR_LRZ },
    { ".tar.br",  VAR_FORMAT_TAR_BR  },
    { ".tar.bz3", VAR_FORMAT_TAR_BZ3 },
    { ".tbz3",    VAR_FORMAT_TAR_BZ3 },
    { ".tar.Z",   VAR_FORMAT_TAR_Z   },
    { ".taz",     VAR_FORMAT_TAR_Z   },
    { ".tar.7z",  VAR_FORMAT_TAR_7Z  },

    /* Archive formats */
    { ".tar",  VAR_FORMAT_TAR    },
    { ".zip",  VAR_FORMAT_ZIP    },
    { ".7z",   VAR_FORMAT_SEVENZ },
    { ".rar",  VAR_FORMAT_RAR    },
    { ".arj",  VAR_FORMAT_ARJ    },
    { ".ace",  VAR_FORMAT_ACE    },
    { ".alz",  VAR_FORMAT_ALZ    },
    { ".ar",   VAR_FORMAT_AR     },
    { ".cab",  VAR_FORMAT_CAB    },
    { ".cpio", VAR_FORMAT_CPIO   },
    { ".lzh",  VAR_FORMAT_LHA    },
    { ".lha",  VAR_FORMAT_LHA    },
    { ".zoo",  VAR_FORMAT_ZOO    },
    { ".jar",  VAR_FORMAT_JAR    },
    { ".war",  VAR_FORMAT_WAR    },
    { ".ear",  VAR_FORMAT_EAR    },
    { ".cbz",  VAR_FORMAT_CBZ    },
    { ".cbr",  VAR_FORMAT_CBR    },

    /* Read-only */
    { ".deb",  VAR_FORMAT_DEB     },
    { ".rpm",  VAR_FORMAT_RPM     },
    { ".iso",  VAR_FORMAT_ISO     },
    { ".dmg",  VAR_FORMAT_DMG     },
    { ".snap", VAR_FORMAT_SNAP    },
    { ".sqsh", VAR_FORMAT_SQUASHFS },
    { ".bin",  VAR_FORMAT_STUFFIT  },
    { ".sit",  VAR_FORMAT_STUFFIT  },

    /* Single-file compression */
    { ".gz",   VAR_FORMAT_GZ   },
    { ".bz2",  VAR_FORMAT_BZ2  },
    { ".bz",   VAR_FORMAT_BZ   },
    { ".xz",   VAR_FORMAT_XZ   },
    { ".zst",  VAR_FORMAT_ZST  },
    { ".lz",   VAR_FORMAT_LZ   },
    { ".lzo",  VAR_FORMAT_LZOP },
    { ".lrz",  VAR_FORMAT_LRZ  },
    { ".br",   VAR_FORMAT_BR   },
    { ".bz3",  VAR_FORMAT_BZ3  },
    { ".Z",    VAR_FORMAT_Z    },
    { ".rz",   VAR_FORMAT_RZ   },

    { NULL, VAR_FORMAT_UNKNOWN },
};

VarFormatType
var_utils_detect_format (const char *filename)
{
    if (!filename) return VAR_FORMAT_UNKNOWN;

    /* Convert to lowercase for comparison (except .Z which is uppercase) */
    for (const ExtMap *m = ext_map; m->extension; m++) {
        size_t elen = strlen (m->extension);
        size_t flen = strlen (filename);
        if (flen >= elen) {
            const char *suffix = filename + flen - elen;
            /* .Z is case-sensitive, everything else case-insensitive */
            if (strcmp (m->extension, ".Z") == 0 || strcmp (m->extension, ".tar.Z") == 0) {
                if (strcmp (suffix, m->extension) == 0)
                    return m->format;
            } else {
                if (strcasecmp (suffix, m->extension) == 0)
                    return m->format;
            }
        }
    }

    return VAR_FORMAT_UNKNOWN;
}

/* ═══════════════════════════════════════════════════
   Format Metadata
   ═══════════════════════════════════════════════════ */

const char *
var_utils_format_name (VarFormatType format)
{
    switch (format) {
    case VAR_FORMAT_ZIP:      return "ZIP Archive";
    case VAR_FORMAT_TAR:      return "TAR Archive";
    case VAR_FORMAT_TAR_GZ:   return "TAR + Gzip";
    case VAR_FORMAT_TAR_BZ2:  return "TAR + Bzip2";
    case VAR_FORMAT_TAR_XZ:   return "TAR + XZ";
    case VAR_FORMAT_TAR_ZST:  return "TAR + Zstandard";
    case VAR_FORMAT_TAR_LZ:   return "TAR + Lzip";
    case VAR_FORMAT_TAR_LZOP: return "TAR + Lzop";
    case VAR_FORMAT_TAR_LRZ:  return "TAR + Lrzip";
    case VAR_FORMAT_TAR_BR:   return "TAR + Brotli";
    case VAR_FORMAT_TAR_BZ:   return "TAR + Bzip";
    case VAR_FORMAT_TAR_BZ3:  return "TAR + Bzip3";
    case VAR_FORMAT_TAR_Z:    return "TAR + Compress";
    case VAR_FORMAT_TAR_7Z:   return "TAR + 7-Zip";
    case VAR_FORMAT_SEVENZ:   return "7-Zip Archive";
    case VAR_FORMAT_RAR:      return "RAR Archive";
    case VAR_FORMAT_ARJ:      return "ARJ Archive";
    case VAR_FORMAT_ACE:      return "ACE Archive";
    case VAR_FORMAT_ALZ:      return "ALZ Archive";
    case VAR_FORMAT_AR:       return "AR Archive";
    case VAR_FORMAT_CAB:      return "Cabinet Archive";
    case VAR_FORMAT_CPIO:     return "CPIO Archive";
    case VAR_FORMAT_LHA:      return "LHA Archive";
    case VAR_FORMAT_ZOO:      return "ZOO Archive";
    case VAR_FORMAT_JAR:      return "Java Archive";
    case VAR_FORMAT_WAR:      return "Java Web Archive";
    case VAR_FORMAT_EAR:      return "Java Enterprise Archive";
    case VAR_FORMAT_CBZ:      return "Comic Book (ZIP)";
    case VAR_FORMAT_CBR:      return "Comic Book (RAR)";
    case VAR_FORMAT_DEB:      return "Debian Package";
    case VAR_FORMAT_RPM:      return "RPM Package";
    case VAR_FORMAT_ISO:      return "ISO 9660 Image";
    case VAR_FORMAT_DMG:      return "Apple Disk Image";
    case VAR_FORMAT_SNAP:     return "Snap Package";
    case VAR_FORMAT_SQUASHFS: return "SquashFS Image";
    case VAR_FORMAT_STUFFIT:  return "StuffIt Archive";
    case VAR_FORMAT_GZ:       return "Gzip Compressed";
    case VAR_FORMAT_BZ2:      return "Bzip2 Compressed";
    case VAR_FORMAT_BZ:       return "Bzip Compressed";
    case VAR_FORMAT_XZ:       return "XZ Compressed";
    case VAR_FORMAT_ZST:      return "Zstandard Compressed";
    case VAR_FORMAT_LZ:       return "Lzip Compressed";
    case VAR_FORMAT_LZOP:     return "Lzop Compressed";
    case VAR_FORMAT_LRZ:      return "Lrzip Compressed";
    case VAR_FORMAT_BR:       return "Brotli Compressed";
    case VAR_FORMAT_BZ3:      return "Bzip3 Compressed";
    case VAR_FORMAT_Z:        return "Unix Compress";
    case VAR_FORMAT_RZ:       return "Rzip Compressed";
    default:                  return "Unknown Format";
    }
}

const char *
var_utils_format_extension (VarFormatType format)
{
    switch (format) {
    case VAR_FORMAT_ZIP:      return ".zip";
    case VAR_FORMAT_TAR:      return ".tar";
    case VAR_FORMAT_TAR_GZ:   return ".tar.gz";
    case VAR_FORMAT_TAR_BZ2:  return ".tar.bz2";
    case VAR_FORMAT_TAR_XZ:   return ".tar.xz";
    case VAR_FORMAT_TAR_ZST:  return ".tar.zst";
    case VAR_FORMAT_TAR_LZ:   return ".tar.lz";
    case VAR_FORMAT_TAR_LZOP: return ".tar.lzo";
    case VAR_FORMAT_TAR_LRZ:  return ".tar.lrz";
    case VAR_FORMAT_TAR_BR:   return ".tar.br";
    case VAR_FORMAT_TAR_BZ:   return ".tar.bz";
    case VAR_FORMAT_TAR_BZ3:  return ".tar.bz3";
    case VAR_FORMAT_TAR_Z:    return ".tar.Z";
    case VAR_FORMAT_TAR_7Z:   return ".tar.7z";
    case VAR_FORMAT_SEVENZ:   return ".7z";
    case VAR_FORMAT_RAR:      return ".rar";
    case VAR_FORMAT_ARJ:      return ".arj";
    case VAR_FORMAT_ACE:      return ".ace";
    case VAR_FORMAT_ALZ:      return ".alz";
    case VAR_FORMAT_AR:       return ".ar";
    case VAR_FORMAT_CAB:      return ".cab";
    case VAR_FORMAT_CPIO:     return ".cpio";
    case VAR_FORMAT_LHA:      return ".lzh";
    case VAR_FORMAT_ZOO:      return ".zoo";
    case VAR_FORMAT_JAR:      return ".jar";
    case VAR_FORMAT_WAR:      return ".war";
    case VAR_FORMAT_EAR:      return ".ear";
    case VAR_FORMAT_CBZ:      return ".cbz";
    case VAR_FORMAT_CBR:      return ".cbr";
    case VAR_FORMAT_DEB:      return ".deb";
    case VAR_FORMAT_RPM:      return ".rpm";
    case VAR_FORMAT_ISO:      return ".iso";
    case VAR_FORMAT_DMG:      return ".dmg";
    case VAR_FORMAT_SNAP:     return ".snap";
    case VAR_FORMAT_SQUASHFS: return ".sqsh";
    case VAR_FORMAT_STUFFIT:  return ".sit";
    case VAR_FORMAT_GZ:       return ".gz";
    case VAR_FORMAT_BZ2:      return ".bz2";
    case VAR_FORMAT_BZ:       return ".bz";
    case VAR_FORMAT_XZ:       return ".xz";
    case VAR_FORMAT_ZST:      return ".zst";
    case VAR_FORMAT_LZ:       return ".lz";
    case VAR_FORMAT_LZOP:     return ".lzo";
    case VAR_FORMAT_LRZ:      return ".lrz";
    case VAR_FORMAT_BR:       return ".br";
    case VAR_FORMAT_BZ3:      return ".bz3";
    case VAR_FORMAT_Z:        return ".Z";
    case VAR_FORMAT_RZ:       return ".rz";
    default:                  return "";
    }
}

gboolean
var_utils_format_is_read_only (VarFormatType format)
{
    switch (format) {
    case VAR_FORMAT_DEB:
    case VAR_FORMAT_RPM:
    case VAR_FORMAT_ISO:
    case VAR_FORMAT_DMG:
    case VAR_FORMAT_SNAP:
    case VAR_FORMAT_SQUASHFS:
    case VAR_FORMAT_STUFFIT:
    case VAR_FORMAT_RAR:
    case VAR_FORMAT_CBR:
    case VAR_FORMAT_ACE:
    case VAR_FORMAT_ALZ:
    case VAR_FORMAT_CAB:
    case VAR_FORMAT_LHA:
        return TRUE;
    default:
        return FALSE;
    }
}

gboolean
var_utils_format_is_single_file (VarFormatType format)
{
    switch (format) {
    case VAR_FORMAT_GZ:
    case VAR_FORMAT_BZ2:
    case VAR_FORMAT_BZ:
    case VAR_FORMAT_XZ:
    case VAR_FORMAT_ZST:
    case VAR_FORMAT_LZ:
    case VAR_FORMAT_LZOP:
    case VAR_FORMAT_LRZ:
    case VAR_FORMAT_BR:
    case VAR_FORMAT_BZ3:
    case VAR_FORMAT_Z:
    case VAR_FORMAT_RZ:
        return TRUE;
    default:
        return FALSE;
    }
}

char *
var_utils_format_ratio (gint64 original, gint64 compressed)
{
    if (original <= 0 || compressed <= 0)
        return g_strdup ("—");

    double ratio = (1.0 - (double)compressed / (double)original) * 100.0;
    if (ratio < 0) ratio = 0;
    return g_strdup_printf ("%.0f%%", ratio);
}
