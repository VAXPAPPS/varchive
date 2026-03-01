/*
 * VArchive - Professional Archive Manager for VAXP-OS
 * Copyright (C) 2026 VAXP Organization
 *
 * var-toolbar.h — Main action toolbar widget
 */

#pragma once

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define VAR_TYPE_TOOLBAR (var_toolbar_get_type ())
G_DECLARE_FINAL_TYPE (VarToolbar, var_toolbar, VAR, TOOLBAR, GtkBox)

VarToolbar *var_toolbar_new                 (void);
void        var_toolbar_set_archive_opened  (VarToolbar *self, gboolean opened);
void        var_toolbar_set_selection_count (VarToolbar *self, int count);
void        var_toolbar_set_search_active   (VarToolbar *self, gboolean active);
const char *var_toolbar_get_search_text     (VarToolbar *self);

/* Signals:
 * "action-open"
 * "action-extract"
 * "action-create"
 * "action-add"
 * "action-delete"
 * "action-test"
 * "action-properties"
 * "search-changed" (const char *text)
 */

G_END_DECLS
