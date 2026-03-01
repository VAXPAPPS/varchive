/*
 * VArchive - Professional Archive Manager for VAXP-OS
 * Copyright (C) 2026 VAXP Organization
 *
 * var-progress-overlay.h — Progress overlay for async operations
 */

#pragma once

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define VAR_TYPE_PROGRESS_OVERLAY (var_progress_overlay_get_type ())
G_DECLARE_FINAL_TYPE (VarProgressOverlay, var_progress_overlay, VAR, PROGRESS_OVERLAY, GtkBox)

VarProgressOverlay *var_progress_overlay_new        (void);
void                var_progress_overlay_show       (VarProgressOverlay *self,
                                                     const char         *title);
void                var_progress_overlay_update     (VarProgressOverlay *self,
                                                     double              fraction,
                                                     const char         *detail);
void                var_progress_overlay_set_fraction (VarProgressOverlay *self,
                                                       double              fraction);
void                var_progress_overlay_set_detail   (VarProgressOverlay *self,
                                                       const char         *detail);
void                var_progress_overlay_hide       (VarProgressOverlay *self);
void                var_progress_overlay_set_cancel_cb (VarProgressOverlay *self,
                                                        GCallback           cb,
                                                        gpointer            user_data);

G_END_DECLS
