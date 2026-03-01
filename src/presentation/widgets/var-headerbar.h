/*
 * VArchive - Professional Archive Manager for VAXP-OS
 * Copyright (C) 2026 VAXP Organization
 *
 * var-headerbar.h — Custom VAXP-OS style window header bar
 */

#pragma once

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define VAR_TYPE_HEADERBAR (var_headerbar_get_type ())
G_DECLARE_FINAL_TYPE (VarHeaderBar, var_headerbar, VAR, HEADERBAR, GtkBox)

VarHeaderBar *var_headerbar_new            (GtkWindow *window);
void          var_headerbar_set_title      (VarHeaderBar *self, const char *title);
void          var_headerbar_set_subtitle   (VarHeaderBar *self, const char *subtitle);
GtkWidget    *var_headerbar_get_start_box  (VarHeaderBar *self);
GtkWidget    *var_headerbar_get_end_box    (VarHeaderBar *self);

G_END_DECLS
