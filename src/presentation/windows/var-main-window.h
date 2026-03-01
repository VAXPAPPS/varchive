/*
 * VArchive - Professional Archive Manager for VAXP-OS
 * Copyright (C) 2026 VAXP Organization
 *
 * var-main-window.h — Main application window
 */

#pragma once

#include <adwaita.h>
#include "../../core/var-application.h"

G_BEGIN_DECLS

#define VAR_TYPE_MAIN_WINDOW (var_main_window_get_type ())
G_DECLARE_FINAL_TYPE (VarMainWindow, var_main_window, VAR, MAIN_WINDOW, AdwApplicationWindow)

VarMainWindow *var_main_window_new      (VarApplication *app);
void           var_main_window_open_file (VarMainWindow *self, GFile *file);

G_END_DECLS
