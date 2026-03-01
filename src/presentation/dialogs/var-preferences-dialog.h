/*
 * VArchive - Professional Archive Manager for VAXP-OS
 * Copyright (C) 2026 VAXP Organization
 *
 * var-preferences-dialog.h — User preferences
 */

#pragma once

#include <adwaita.h>

G_BEGIN_DECLS

#define VAR_TYPE_PREFERENCES_DIALOG (var_preferences_dialog_get_type ())
G_DECLARE_FINAL_TYPE (VarPreferencesDialog, var_preferences_dialog, VAR, PREFERENCES_DIALOG, AdwPreferencesWindow)

VarPreferencesDialog *var_preferences_dialog_new (GtkWindow *parent);

G_END_DECLS
