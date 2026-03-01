/*
 * VArchive - Professional Archive Manager for VAXP-OS
 * Copyright (C) 2026 VAXP Organization
 *
 * var-archive-service.h — High-level archive operations header
 */

#pragma once

#include <gio/gio.h>
#include "../core/var-types.h"
#include "../backend/var-entry.h"

G_BEGIN_DECLS

#define VAR_TYPE_ARCHIVE_SERVICE (var_archive_service_get_type ())
G_DECLARE_FINAL_TYPE (VarArchiveService, var_archive_service,
                       VAR, ARCHIVE_SERVICE, GObject)

VarArchiveService *var_archive_service_get_default   (void);

/* Open and list archive contents (async) */
void     var_archive_service_open_async     (VarArchiveService    *self,
                                              const char           *path,
                                              const char           *password,
                                              GCancellable         *cancellable,
                                              GAsyncReadyCallback   callback,
                                              gpointer              user_data);
GPtrArray *var_archive_service_open_finish   (VarArchiveService    *self,
                                              GAsyncResult         *result,
                                              GError              **error);

/* Extract (async) */
void     var_archive_service_extract_async  (VarArchiveService    *self,
                                              const char           *archive_path,
                                              const char           *dest_dir,
                                              GPtrArray            *files,
                                              const char           *password,
                                              VarOverwritePolicy    overwrite,
                                              GCancellable         *cancellable,
                                              GAsyncReadyCallback   callback,
                                              gpointer              user_data);
gboolean  var_archive_service_extract_finish (VarArchiveService    *self,
                                              GAsyncResult         *result,
                                              GError              **error);

/* Create (async) */
void     var_archive_service_create_async   (VarArchiveService    *self,
                                              const char           *archive_path,
                                              VarFormatType         format,
                                              GPtrArray            *files,
                                              const char           *base_dir,
                                              const char           *password,
                                              VarCompressionLevel   level,
                                              GCancellable         *cancellable,
                                              GAsyncReadyCallback   callback,
                                              gpointer              user_data);
gboolean  var_archive_service_create_finish  (VarArchiveService    *self,
                                              GAsyncResult         *result,
                                              GError              **error);

/* Signals: "progress-changed" (double fraction, const char *file) */

G_END_DECLS
