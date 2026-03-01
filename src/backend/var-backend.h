/*
 * VArchive - Professional Archive Manager for VAXP-OS
 * Copyright (C) 2026 VAXP Organization
 *
 * var-backend.h — Abstract backend interface header
 */

#pragma once

#include <glib-object.h>
#include "../core/var-types.h"
#include "var-entry.h"

G_BEGIN_DECLS

#include <gio/gio.h>

#define VAR_TYPE_BACKEND (var_backend_get_type ())
G_DECLARE_INTERFACE (VarBackend, var_backend, VAR, BACKEND, GObject)

struct _VarBackendInterface {
    GTypeInterface  parent_iface;

    /* List all entries in the archive */
    GPtrArray *  (*list)     (VarBackend           *self,
                              const char           *archive_path,
                              const char           *password,
                              GError              **error);

    /* Extract files from archive */
    gboolean     (*extract)  (VarBackend           *self,
                              const char           *archive_path,
                              const char           *dest_dir,
                              GPtrArray            *files_to_extract,  /* NULL = all */
                              const char           *password,
                              VarOverwritePolicy    overwrite,
                              VarProgressCallback   progress_cb,
                              gpointer              progress_data,
                              GCancellable         *cancellable,
                              GError              **error);

    /* Create a new archive from file list */
    gboolean     (*create)   (VarBackend           *self,
                              const char           *archive_path,
                              VarFormatType         format,
                              GPtrArray            *files,          /* GFile* array */
                              const char           *base_dir,
                              const char           *password,
                              VarCompressionLevel   level,
                              VarProgressCallback   progress_cb,
                              gpointer              progress_data,
                              GCancellable         *cancellable,
                              GError              **error);

    /* Add files to existing archive */
    gboolean     (*add)      (VarBackend           *self,
                              const char           *archive_path,
                              GPtrArray            *files,
                              const char           *base_dir,
                              const char           *password,
                              GCancellable         *cancellable,
                              GError              **error);

    /* Delete files from archive */
    gboolean     (*delete_entries) (VarBackend     *self,
                              const char           *archive_path,
                              GPtrArray            *entry_paths,
                              GCancellable         *cancellable,
                              GError              **error);

    /* Test archive integrity */
    gboolean     (*test)     (VarBackend           *self,
                              const char           *archive_path,
                              const char           *password,
                              GError              **error);
};

/* Convenience wrappers */
GPtrArray * var_backend_list           (VarBackend *self, const char *path,
                                        const char *password, GError **error);
gboolean    var_backend_extract        (VarBackend *self, const char *archive,
                                        const char *dest, GPtrArray *files,
                                        const char *password, VarOverwritePolicy ow,
                                        VarProgressCallback cb, gpointer data,
                                        GCancellable *cancel, GError **error);
gboolean    var_backend_create         (VarBackend *self, const char *archive,
                                        VarFormatType fmt, GPtrArray *files,
                                        const char *base_dir, const char *password,
                                        VarCompressionLevel level,
                                        VarProgressCallback cb, gpointer data,
                                        GCancellable *cancel, GError **error);
gboolean    var_backend_add            (VarBackend *self, const char *archive,
                                        GPtrArray *files, const char *base_dir,
                                        const char *password,
                                        GCancellable *cancel, GError **error);
gboolean    var_backend_delete_entries (VarBackend *self, const char *archive,
                                        GPtrArray *paths,
                                        GCancellable *cancel, GError **error);
gboolean    var_backend_test           (VarBackend *self, const char *archive,
                                        const char *password, GError **error);

G_END_DECLS
