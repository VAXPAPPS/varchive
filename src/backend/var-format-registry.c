/*
 * VArchive - Professional Archive Manager for VAXP-OS
 * Copyright (C) 2026 VAXP Organization
 *
 * var-format-registry.c — Maps formats to backends
 */

#include "var-format-registry.h"
#include "../core/var-utils.h"

struct _VarFormatRegistry {
    GObject               parent_instance;
    VarLibarchiveBackend *la_backend;
    VarCommandBackend    *cmd_backend;
};

G_DEFINE_TYPE (VarFormatRegistry, var_format_registry, G_TYPE_OBJECT)

static VarFormatRegistry *default_registry = NULL;

/* ── Determine if libarchive handles this format ── */

static gboolean
is_libarchive_format (VarFormatType fmt)
{
    switch (fmt) {
    case VAR_FORMAT_ZIP:  case VAR_FORMAT_TAR:    case VAR_FORMAT_TAR_GZ:
    case VAR_FORMAT_TAR_BZ2: case VAR_FORMAT_TAR_BZ: case VAR_FORMAT_TAR_XZ:
    case VAR_FORMAT_TAR_ZST: case VAR_FORMAT_TAR_LZ: case VAR_FORMAT_TAR_Z:
    case VAR_FORMAT_TAR_7Z:
    case VAR_FORMAT_SEVENZ: case VAR_FORMAT_RAR:    case VAR_FORMAT_AR:
    case VAR_FORMAT_CAB:    case VAR_FORMAT_CPIO:   case VAR_FORMAT_LHA:
    case VAR_FORMAT_DEB:    case VAR_FORMAT_RPM:    case VAR_FORMAT_ISO:
    case VAR_FORMAT_JAR:    case VAR_FORMAT_WAR:    case VAR_FORMAT_EAR:
    case VAR_FORMAT_CBZ:    case VAR_FORMAT_CBR:
    case VAR_FORMAT_GZ:     case VAR_FORMAT_BZ2:    case VAR_FORMAT_BZ:
    case VAR_FORMAT_XZ:     case VAR_FORMAT_ZST:    case VAR_FORMAT_LZ:
    case VAR_FORMAT_Z:
        return TRUE;
    default:
        return FALSE;
    }
}

/* ── Public API ──────────────────────────────────── */

VarBackend *
var_format_registry_get_backend (VarFormatRegistry *self, const char *filename)
{
    VarFormatType fmt = var_utils_detect_format (filename);
    return var_format_registry_get_backend_for_format (self, fmt);
}

VarBackend *
var_format_registry_get_backend_for_format (VarFormatRegistry *self,
                                             VarFormatType format)
{
    if (is_libarchive_format (format))
        return VAR_BACKEND (self->la_backend);
    else
        return VAR_BACKEND (self->cmd_backend);
}

VarCapabilityFlags
var_format_registry_get_caps (VarFormatRegistry *self, VarFormatType format)
{
    (void)self;
    VarCapabilityFlags caps = VAR_CAP_READ | VAR_CAP_LIST;

    if (!var_utils_format_is_read_only (format))
        caps |= VAR_CAP_WRITE | VAR_CAP_ADD | VAR_CAP_DELETE;

    /* Password support for known formats */
    switch (format) {
    case VAR_FORMAT_ZIP: case VAR_FORMAT_SEVENZ: case VAR_FORMAT_RAR:
    case VAR_FORMAT_CBR: case VAR_FORMAT_CBZ:
        caps |= VAR_CAP_PASSWORD;
        break;
    default:
        break;
    }

    if (is_libarchive_format (format))
        caps |= VAR_CAP_TEST;

    return caps;
}

/* ── Lifecycle ──────────────────────────────────── */

static void
var_format_registry_finalize (GObject *obj)
{
    VarFormatRegistry *self = VAR_FORMAT_REGISTRY (obj);
    g_clear_object (&self->la_backend);
    g_clear_object (&self->cmd_backend);
    G_OBJECT_CLASS (var_format_registry_parent_class)->finalize (obj);
}

static void
var_format_registry_class_init (VarFormatRegistryClass *klass)
{
    G_OBJECT_CLASS (klass)->finalize = var_format_registry_finalize;
}

static void
var_format_registry_init (VarFormatRegistry *self)
{
    self->la_backend  = var_libarchive_backend_new ();
    self->cmd_backend = var_command_backend_new ();
}

VarFormatRegistry *
var_format_registry_get_default (void)
{
    if (!default_registry)
        default_registry = g_object_new (VAR_TYPE_FORMAT_REGISTRY, NULL);
    return default_registry;
}
