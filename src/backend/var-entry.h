/*
 * VArchive - Professional Archive Manager for VAXP-OS
 * Copyright (C) 2026 VAXP Organization
 *
 * var-entry.h — Archive entry model header
 */

#pragma once

#include <glib-object.h>
#include "../core/var-types.h"

G_BEGIN_DECLS

#define VAR_TYPE_ENTRY (var_entry_get_type ())
G_DECLARE_FINAL_TYPE (VarEntry, var_entry, VAR, ENTRY, GObject)

VarEntry      *var_entry_new            (void);
VarEntry      *var_entry_new_full       (const char   *path,
                                         VarEntryType  type,
                                         gint64        size,
                                         gint64        compressed_size,
                                         gint64        mod_time,
                                         guint32       permissions,
                                         gboolean      is_encrypted);

const char    *var_entry_get_path       (VarEntry *self);
void           var_entry_set_path       (VarEntry *self, const char *path);
const char    *var_entry_get_filename   (VarEntry *self);
VarEntryType   var_entry_get_entry_type (VarEntry *self);
gint64         var_entry_get_size       (VarEntry *self);
gint64         var_entry_get_compressed (VarEntry *self);
gint64         var_entry_get_mod_time   (VarEntry *self);
guint32        var_entry_get_permissions(VarEntry *self);
gboolean       var_entry_get_encrypted  (VarEntry *self);

/* Tree support */
void           var_entry_add_child      (VarEntry *self, VarEntry *child);
GPtrArray     *var_entry_get_children   (VarEntry *self);
VarEntry      *var_entry_get_parent     (VarEntry *self);
void           var_entry_set_parent     (VarEntry *self, VarEntry *parent);

G_END_DECLS
