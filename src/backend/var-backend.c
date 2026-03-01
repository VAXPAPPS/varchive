/*
 * VArchive - Professional Archive Manager for VAXP-OS
 * Copyright (C) 2026 VAXP Organization
 *
 * var-backend.c — Abstract backend interface implementation
 */

#include "var-backend.h"

G_DEFINE_INTERFACE (VarBackend, var_backend, G_TYPE_OBJECT)

static void
var_backend_default_init (VarBackendInterface *iface)
{
    (void)iface;
}

GPtrArray *
var_backend_list (VarBackend *self, const char *path,
                  const char *password, GError **error)
{
    VarBackendInterface *iface = VAR_BACKEND_GET_IFACE (self);
    g_return_val_if_fail (iface->list != NULL, NULL);
    return iface->list (self, path, password, error);
}

gboolean
var_backend_extract (VarBackend *self, const char *archive, const char *dest,
                     GPtrArray *files, const char *password,
                     VarOverwritePolicy ow, VarProgressCallback cb,
                     gpointer data, GCancellable *cancel, GError **error)
{
    VarBackendInterface *iface = VAR_BACKEND_GET_IFACE (self);
    g_return_val_if_fail (iface->extract != NULL, FALSE);
    return iface->extract (self, archive, dest, files, password, ow, cb, data, cancel, error);
}

gboolean
var_backend_create (VarBackend *self, const char *archive, VarFormatType fmt,
                    GPtrArray *files, const char *base_dir, const char *password,
                    VarCompressionLevel level, VarProgressCallback cb,
                    gpointer data, GCancellable *cancel, GError **error)
{
    VarBackendInterface *iface = VAR_BACKEND_GET_IFACE (self);
    g_return_val_if_fail (iface->create != NULL, FALSE);
    return iface->create (self, archive, fmt, files, base_dir, password, level, cb, data, cancel, error);
}

gboolean
var_backend_add (VarBackend *self, const char *archive, GPtrArray *files,
                 const char *base_dir, const char *password,
                 GCancellable *cancel, GError **error)
{
    VarBackendInterface *iface = VAR_BACKEND_GET_IFACE (self);
    g_return_val_if_fail (iface->add != NULL, FALSE);
    return iface->add (self, archive, files, base_dir, password, cancel, error);
}

gboolean
var_backend_delete_entries (VarBackend *self, const char *archive,
                            GPtrArray *paths, GCancellable *cancel,
                            GError **error)
{
    VarBackendInterface *iface = VAR_BACKEND_GET_IFACE (self);
    g_return_val_if_fail (iface->delete_entries != NULL, FALSE);
    return iface->delete_entries (self, archive, paths, cancel, error);
}

gboolean
var_backend_test (VarBackend *self, const char *archive,
                  const char *password, GError **error)
{
    VarBackendInterface *iface = VAR_BACKEND_GET_IFACE (self);
    g_return_val_if_fail (iface->test != NULL, FALSE);
    return iface->test (self, archive, password, error);
}
