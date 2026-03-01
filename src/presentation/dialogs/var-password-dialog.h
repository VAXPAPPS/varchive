/*
 * VArchive - Professional Archive Manager for VAXP-OS
 * Copyright (C) 2026 VAXP Organization
 *
 * var-password-dialog.h — Password prompt dialog
 */

#pragma once

#include <adwaita.h>

G_BEGIN_DECLS

#define VAR_TYPE_PASSWORD_DIALOG (var_password_dialog_get_type ())
G_DECLARE_FINAL_TYPE (VarPasswordDialog, var_password_dialog, VAR, PASSWORD_DIALOG, AdwWindow)

typedef void (*VarPasswordCallback) (const char *password, gpointer user_data);

VarPasswordDialog *var_password_dialog_new (GtkWindow *parent, const char *archive_name);

void var_password_dialog_set_callback (VarPasswordDialog   *self,
                                       VarPasswordCallback  callback,
                                       gpointer             user_data);

G_END_DECLS
