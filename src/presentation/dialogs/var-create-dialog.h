/*
 * VArchive - Professional Archive Manager for VAXP-OS
 * Copyright (C) 2026 VAXP Organization
 *
 * var-create-dialog.h — Archive creation dialog
 */

#pragma once

#include <adwaita.h>
#include "../../core/var-types.h"

G_BEGIN_DECLS

#define VAR_TYPE_CREATE_DIALOG (var_create_dialog_get_type ())
G_DECLARE_FINAL_TYPE (VarCreateDialog, var_create_dialog, VAR, CREATE_DIALOG, AdwWindow)

// Callback type for when creation is confirmed
typedef void (*VarCreateSettingsCallback) (const char          *dest_path,
                                           VarFormatType        format,
                                           VarCompressionLevel  level,
                                           const char          *password,
                                           gpointer             user_data);

VarCreateDialog *var_create_dialog_new        (GtkWindow                   *parent,
                                               const char                  *suggested_name);

void             var_create_dialog_set_callback(VarCreateDialog            *self,
                                               VarCreateSettingsCallback    callback,
                                               gpointer                     user_data);

G_END_DECLS
