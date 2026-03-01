/*
 * VArchive - Professional Archive Manager for VAXP-OS
 * Copyright (C) 2026 VAXP Organization
 *
 * var-entry.c — Archive entry model implementation
 */

#include "var-entry.h"
#include <string.h>

struct _VarEntry {
    GObject       parent_instance;

    char         *path;
    char         *filename;      /* basename derived from path */
    VarEntryType  entry_type;
    gint64        size;
    gint64        compressed_size;
    gint64        mod_time;
    guint32       permissions;
    gboolean      is_encrypted;

    /* Tree */
    VarEntry     *parent;
    GPtrArray    *children;
};

G_DEFINE_TYPE (VarEntry, var_entry, G_TYPE_OBJECT)

enum {
    PROP_0,
    PROP_FILENAME,
    N_PROPERTIES
};

static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };

static void
var_entry_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
    VarEntry *self = VAR_ENTRY (object);
    switch (property_id) {
    case PROP_FILENAME:
        g_value_set_string (value, self->filename);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        break;
    }
}

static void
var_entry_finalize (GObject *obj)
{
    VarEntry *self = VAR_ENTRY (obj);
    g_free (self->path);
    g_free (self->filename);
    if (self->children)
        g_ptr_array_unref (self->children);
    G_OBJECT_CLASS (var_entry_parent_class)->finalize (obj);
}

static void
var_entry_class_init (VarEntryClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
    gobject_class->finalize = var_entry_finalize;
    gobject_class->get_property = var_entry_get_property;

    obj_properties[PROP_FILENAME] =
        g_param_spec_string ("filename",
                             "Filename",
                             "Basename of the entry",
                             NULL,
                             G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

    g_object_class_install_properties (gobject_class, N_PROPERTIES, obj_properties);
}

static void
var_entry_init (VarEntry *self)
{
    self->path            = NULL;
    self->filename        = NULL;
    self->entry_type      = VAR_ENTRY_FILE;
    self->size            = -1;
    self->compressed_size = -1;
    self->mod_time        = 0;
    self->permissions     = 0;
    self->is_encrypted    = FALSE;
    self->parent          = NULL;
    self->children        = NULL;
}

VarEntry *
var_entry_new (void)
{
    return g_object_new (VAR_TYPE_ENTRY, NULL);
}

VarEntry *
var_entry_new_full (const char   *path,
                    VarEntryType  type,
                    gint64        size,
                    gint64        compressed_size,
                    gint64        mod_time,
                    guint32       permissions,
                    gboolean      is_encrypted)
{
    VarEntry *self = var_entry_new ();
    var_entry_set_path (self, path);
    self->entry_type      = type;
    self->size            = size;
    self->compressed_size = compressed_size;
    self->mod_time        = mod_time;
    self->permissions     = permissions;
    self->is_encrypted    = is_encrypted;
    return self;
}

const char *
var_entry_get_path (VarEntry *self)
{
    g_return_val_if_fail (VAR_IS_ENTRY (self), NULL);
    return self->path;
}

void
var_entry_set_path (VarEntry *self, const char *path)
{
    g_return_if_fail (VAR_IS_ENTRY (self));
    g_free (self->path);
    g_free (self->filename);
    self->path = g_strdup (path);

    /* Derive filename (basename) */
    if (path) {
        /* Remove trailing slash for directories */
        char *clean = g_strdup (path);
        size_t len = strlen (clean);
        if (len > 1 && clean[len - 1] == '/')
            clean[len - 1] = '\0';

        char *base = strrchr (clean, '/');
        self->filename = g_strdup (base ? base + 1 : clean);
        g_free (clean);
    } else {
        self->filename = NULL;
    }
    g_object_notify_by_pspec (G_OBJECT (self), obj_properties[PROP_FILENAME]);
}

const char    *var_entry_get_filename    (VarEntry *self) { return self->filename; }
VarEntryType   var_entry_get_entry_type  (VarEntry *self) { return self->entry_type; }
gint64         var_entry_get_size        (VarEntry *self) { return self->size; }
gint64         var_entry_get_compressed  (VarEntry *self) { return self->compressed_size; }
gint64         var_entry_get_mod_time    (VarEntry *self) { return self->mod_time; }
guint32        var_entry_get_permissions (VarEntry *self) { return self->permissions; }
gboolean       var_entry_get_encrypted   (VarEntry *self) { return self->is_encrypted; }

/* ── Tree ─────────────────────────────────────── */

void
var_entry_add_child (VarEntry *self, VarEntry *child)
{
    g_return_if_fail (VAR_IS_ENTRY (self));
    g_return_if_fail (VAR_IS_ENTRY (child));

    if (!self->children)
        self->children = g_ptr_array_new_with_free_func (g_object_unref);

    child->parent = self;
    g_ptr_array_add (self->children, g_object_ref (child));
}

GPtrArray *
var_entry_get_children (VarEntry *self)
{
    g_return_val_if_fail (VAR_IS_ENTRY (self), NULL);
    return self->children;
}

VarEntry *
var_entry_get_parent (VarEntry *self)
{
    g_return_val_if_fail (VAR_IS_ENTRY (self), NULL);
    return self->parent;
}

void
var_entry_set_parent (VarEntry *self, VarEntry *parent)
{
    g_return_if_fail (VAR_IS_ENTRY (self));
    self->parent = parent;
}
