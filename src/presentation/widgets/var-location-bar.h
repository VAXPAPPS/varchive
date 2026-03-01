/*
 * VArchive - Professional Archive Manager for VAXP-OS
 * Copyright (C) 2026 VAXP Organization
 *
 * var-location-bar.h — Breadcrumb navigation bar for archives
 */

#pragma once

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define VAR_TYPE_LOCATION_BAR (var_location_bar_get_type ())
G_DECLARE_FINAL_TYPE (VarLocationBar, var_location_bar, VAR, LOCATION_BAR, GtkBox)

VarLocationBar *var_location_bar_new           (void);
void            var_location_bar_set_path      (VarLocationBar *self, const char *path);
const char     *var_location_bar_get_path      (VarLocationBar *self);

/* Signal emitted when user clicks a path segment: "path-changed" (const char *path) */

G_END_DECLS
