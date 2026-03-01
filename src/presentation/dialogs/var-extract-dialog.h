/*
 * VArchive - Professional Archive Manager for VAXP-OS
 * Copyright (C) 2026 VAXP Organization
 *
 * var-extract-dialog.h — Archive extraction dialog
 */

#pragma once

#include <adwaita.h>
#include "../../core/var-types.h"

G_BEGIN_DECLS

#define VAR_TYPE_EXTRACT_DIALOG (var_extract_dialog_get_type ())
G_DECLARE_FINAL_TYPE (VarExtractDialog, var_extract_dialog, VAR, EXTRACT_DIALOG, AdwWindow)

// Callback type for when extraction is confirmed
typedef void (*VarExtractSettingsCallback) (const char         *dest_path,
                                            gboolean            extract_all,
                                            gboolean            create_folder,
                                            VarOverwritePolicy  overwrite,
                                            gpointer            user_data);

VarExtractDialog *var_extract_dialog_new  (GtkWindow                  *parent,
                                           const char                 *archive_path,
                                           gboolean                    has_selection);

void var_extract_dialog_set_callback      (VarExtractDialog           *self,
                                           VarExtractSettingsCallback  callback,
                                           gpointer                    user_data);

G_END_DECLS
