/*
 * VArchive - Professional Archive Manager for VAXP-OS
 * Copyright (C) 2026 VAXP Organization
 *
 * var-properties-dialog.h — Archive properties panel
 */

#pragma once

#include <adwaita.h>

G_BEGIN_DECLS

#define VAR_TYPE_PROPERTIES_DIALOG (var_properties_dialog_get_type ())
G_DECLARE_FINAL_TYPE (VarPropertiesDialog, var_properties_dialog, VAR, PROPERTIES_DIALOG, AdwWindow)

VarPropertiesDialog *var_properties_dialog_new (GtkWindow *parent,
                                                const char *archive_path,
                                                const char *format_name,
                                                guint64     total_size,
                                                guint64     compressed_size,
                                                guint       file_count,
                                                guint       dir_count);

G_END_DECLS
