/*
 * VArchive - Professional Archive Manager for VAXP-OS
 * Copyright (C) 2026 VAXP Organization
 *
 * var-archive-service.c — High-level archive operations
 */

#include "var-archive-service.h"
#include "../backend/var-format-registry.h"
#include "../core/var-utils.h"

struct _VarArchiveService {
    GObject            parent_instance;
    VarFormatRegistry *registry;
};

G_DEFINE_TYPE (VarArchiveService, var_archive_service, G_TYPE_OBJECT)

enum {
    SIGNAL_PROGRESS,
    N_SIGNALS
};

static guint signals[N_SIGNALS];
static VarArchiveService *default_service = NULL;

/* ═══════════════════════════════════════════════════
   Progress relay — called from backend thread
   ═══════════════════════════════════════════════════ */

typedef struct {
    VarArchiveService *service;
    double             fraction;
    char              *current_file;
} ProgressInfo;

static gboolean
emit_progress_idle (gpointer data)
{
    ProgressInfo *pi = data;
    g_signal_emit (pi->service, signals[SIGNAL_PROGRESS], 0,
                   pi->fraction, pi->current_file);
    g_free (pi->current_file);
    g_free (pi);
    return G_SOURCE_REMOVE;
}

static void
service_progress_cb (double fraction, const char *file,
                     gint64 done, gint64 total, gpointer user_data)
{
    (void)done; (void)total;
    VarArchiveService *self = VAR_ARCHIVE_SERVICE (user_data);

    ProgressInfo *pi = g_new0 (ProgressInfo, 1);
    pi->service      = self;
    pi->fraction     = fraction;
    pi->current_file = g_strdup (file);

    g_idle_add (emit_progress_idle, pi);
}

/* ═══════════════════════════════════════════════════
   Open (async)
   ═══════════════════════════════════════════════════ */

typedef struct {
    char *path;
    char *password;
} OpenData;

static void
open_data_free (gpointer data)
{
    OpenData *od = data;
    g_free (od->path);
    g_free (od->password);
    g_free (od);
}

static void
open_thread (GTask *task, gpointer src, gpointer task_data, GCancellable *cancel)
{
    (void)cancel;
    VarArchiveService *self = VAR_ARCHIVE_SERVICE (src);
    OpenData *od = task_data;

    VarBackend *backend = var_format_registry_get_backend (self->registry, od->path);
    if (!backend) {
        g_task_return_new_error (task, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED,
                                 "Unsupported archive format");
        return;
    }

    GError *error = NULL;
    GPtrArray *entries = var_backend_list (backend, od->path, od->password, &error);
    if (error) {
        g_task_return_error (task, error);
        return;
    }

    g_task_return_pointer (task, entries, (GDestroyNotify)g_ptr_array_unref);
}

void
var_archive_service_open_async (VarArchiveService *self, const char *path,
                                 const char *password, GCancellable *cancellable,
                                 GAsyncReadyCallback callback, gpointer user_data)
{
    GTask *task = g_task_new (self, cancellable, callback, user_data);

    OpenData *od = g_new0 (OpenData, 1);
    od->path     = g_strdup (path);
    od->password = g_strdup (password);
    g_task_set_task_data (task, od, open_data_free);

    g_task_run_in_thread (task, open_thread);
    g_object_unref (task);
}

GPtrArray *
var_archive_service_open_finish (VarArchiveService *self, GAsyncResult *result,
                                  GError **error)
{
    (void)self;
    return g_task_propagate_pointer (G_TASK (result), error);
}

/* ═══════════════════════════════════════════════════
   Extract (async)
   ═══════════════════════════════════════════════════ */

typedef struct {
    char              *archive_path;
    char              *dest_dir;
    GPtrArray         *files;
    char              *password;
    VarOverwritePolicy overwrite;
} ExtractData;

static void
extract_data_free (gpointer data)
{
    ExtractData *ed = data;
    g_free (ed->archive_path);
    g_free (ed->dest_dir);
    if (ed->files) g_ptr_array_unref (ed->files);
    g_free (ed->password);
    g_free (ed);
}

static void
extract_thread (GTask *task, gpointer src, gpointer task_data, GCancellable *cancel)
{
    VarArchiveService *self = VAR_ARCHIVE_SERVICE (src);
    ExtractData *ed = task_data;

    VarBackend *backend = var_format_registry_get_backend (self->registry, ed->archive_path);
    if (!backend) {
        g_task_return_new_error (task, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED,
                                 "Unsupported format");
        return;
    }

    GError *error = NULL;
    gboolean ok = var_backend_extract (backend, ed->archive_path, ed->dest_dir,
                                        ed->files, ed->password, ed->overwrite,
                                        service_progress_cb, self,
                                        cancel, &error);
    if (error)
        g_task_return_error (task, error);
    else
        g_task_return_boolean (task, ok);
}

void
var_archive_service_extract_async (VarArchiveService *self, const char *archive,
                                    const char *dest, GPtrArray *files,
                                    const char *password, VarOverwritePolicy ow,
                                    GCancellable *cancel, GAsyncReadyCallback cb,
                                    gpointer user_data)
{
    GTask *task = g_task_new (self, cancel, cb, user_data);

    ExtractData *ed     = g_new0 (ExtractData, 1);
    ed->archive_path    = g_strdup (archive);
    ed->dest_dir        = g_strdup (dest);
    ed->files           = files ? g_ptr_array_ref (files) : NULL;
    ed->password        = g_strdup (password);
    ed->overwrite       = ow;
    g_task_set_task_data (task, ed, extract_data_free);

    g_task_run_in_thread (task, extract_thread);
    g_object_unref (task);
}

gboolean
var_archive_service_extract_finish (VarArchiveService *self, GAsyncResult *result,
                                     GError **error)
{
    (void)self;
    return g_task_propagate_boolean (G_TASK (result), error);
}

/* ═══════════════════════════════════════════════════
   Create (async)
   ═══════════════════════════════════════════════════ */

typedef struct {
    char               *archive_path;
    VarFormatType       format;
    GPtrArray          *files;
    char               *base_dir;
    char               *password;
    VarCompressionLevel level;
} CreateData;

static void
create_data_free (gpointer data)
{
    CreateData *cd = data;
    g_free (cd->archive_path);
    if (cd->files) g_ptr_array_unref (cd->files);
    g_free (cd->base_dir);
    g_free (cd->password);
    g_free (cd);
}

static void
create_thread (GTask *task, gpointer src, gpointer task_data, GCancellable *cancel)
{
    VarArchiveService *self = VAR_ARCHIVE_SERVICE (src);
    CreateData *cd = task_data;

    VarBackend *backend = var_format_registry_get_backend_for_format (
        self->registry, cd->format);
    if (!backend) {
        g_task_return_new_error (task, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED,
                                 "Unsupported format for creation");
        return;
    }

    GError *error = NULL;
    gboolean ok = var_backend_create (backend, cd->archive_path, cd->format,
                                       cd->files, cd->base_dir, cd->password,
                                       cd->level, service_progress_cb, self,
                                       cancel, &error);
    if (error)
        g_task_return_error (task, error);
    else
        g_task_return_boolean (task, ok);
}

void
var_archive_service_create_async (VarArchiveService *self, const char *archive,
                                   VarFormatType format, GPtrArray *files,
                                   const char *base_dir, const char *password,
                                   VarCompressionLevel level,
                                   GCancellable *cancel, GAsyncReadyCallback cb,
                                   gpointer user_data)
{
    GTask *task = g_task_new (self, cancel, cb, user_data);

    CreateData *cd    = g_new0 (CreateData, 1);
    cd->archive_path  = g_strdup (archive);
    cd->format        = format;
    cd->files         = files ? g_ptr_array_ref (files) : NULL;
    cd->base_dir      = g_strdup (base_dir);
    cd->password      = g_strdup (password);
    cd->level         = level;
    g_task_set_task_data (task, cd, create_data_free);

    g_task_run_in_thread (task, create_thread);
    g_object_unref (task);
}

gboolean
var_archive_service_create_finish (VarArchiveService *self, GAsyncResult *result,
                                    GError **error)
{
    (void)self;
    return g_task_propagate_boolean (G_TASK (result), error);
}

/* ═══════════════════════════════════════════════════
   Lifecycle
   ═══════════════════════════════════════════════════ */

static void
var_archive_service_finalize (GObject *obj)
{
    VarArchiveService *self = VAR_ARCHIVE_SERVICE (obj);
    g_clear_object (&self->registry);
    G_OBJECT_CLASS (var_archive_service_parent_class)->finalize (obj);
}

static void
var_archive_service_class_init (VarArchiveServiceClass *klass)
{
    GObjectClass *obj_class = G_OBJECT_CLASS (klass);
    obj_class->finalize = var_archive_service_finalize;

    signals[SIGNAL_PROGRESS] = g_signal_new (
        "progress-changed",
        G_TYPE_FROM_CLASS (klass),
        G_SIGNAL_RUN_LAST,
        0, NULL, NULL, NULL,
        G_TYPE_NONE, 2,
        G_TYPE_DOUBLE,
        G_TYPE_STRING
    );
}

static void
var_archive_service_init (VarArchiveService *self)
{
    self->registry = g_object_ref (var_format_registry_get_default ());
}

VarArchiveService *
var_archive_service_get_default (void)
{
    if (!default_service)
        default_service = g_object_new (VAR_TYPE_ARCHIVE_SERVICE, NULL);
    return default_service;
}
