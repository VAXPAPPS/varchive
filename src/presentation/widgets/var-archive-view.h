/*
 * VArchive - Professional Archive Manager for VAXP-OS
 * Copyright (C) 2026 VAXP Organization
 *
 * var-archive-view.h — Archive contents tree view
 */

#pragma once

#include <gtk/gtk.h>
#include "../../backend/var-entry.h"

G_BEGIN_DECLS

#define VAR_TYPE_ARCHIVE_VIEW (var_archive_view_get_type ())
G_DECLARE_FINAL_TYPE (VarArchiveView, var_archive_view, VAR, ARCHIVE_VIEW, GtkBox)

VarArchiveView *var_archive_view_new                (void);
void            var_archive_view_set_model          (VarArchiveView *self, GPtrArray *entries);
void            var_archive_view_clear              (VarArchiveView *self);
void            var_archive_view_set_current_folder (VarArchiveView *self, const char *path);
const char     *var_archive_view_get_current_folder (VarArchiveView *self);
GPtrArray      *var_archive_view_get_selected_paths (VarArchiveView *self);
void            var_archive_view_set_search_filter  (VarArchiveView *self, const char *text);
gboolean        var_archive_view_has_selection      (VarArchiveView *self);

/* Signals:
 * "folder-activated" (const char *path)
 * "file-activated" (const char *path)
 * "selection-changed" (int count)
 */

G_END_DECLS
