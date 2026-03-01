/*
 * VArchive - Professional Archive Manager for VAXP-OS
 * Copyright (C) 2026 VAXP Organization
 *
 * var-libarchive-backend.c — Primary backend using libarchive
 */

#include "var-libarchive-backend.h"
#include <archive.h>
#include <archive_entry.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <glib/gstdio.h>
#include "../core/var-utils.h"

struct _VarLibarchiveBackend {
    GObject  parent_instance;
};

static void var_libarchive_backend_iface_init (VarBackendInterface *iface);

G_DEFINE_TYPE_WITH_CODE (VarLibarchiveBackend, var_libarchive_backend, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE (VAR_TYPE_BACKEND, var_libarchive_backend_iface_init))

/* ═══════════════════════════════════════════════════
   Helper: configure archive reader for all formats
   ═══════════════════════════════════════════════════ */

static struct archive *
create_reader (const char *password)
{
    struct archive *a = archive_read_new ();
    archive_read_support_filter_all (a);
    archive_read_support_format_all (a);

    if (password && password[0])
        archive_read_add_passphrase (a, password);

    return a;
}

/* ═══════════════════════════════════════════════════
   List
   ═══════════════════════════════════════════════════ */

static GPtrArray *
la_list (VarBackend *backend, const char *archive_path,
         const char *password, GError **error)
{
    (void)backend;
    struct archive *a = create_reader (password);
    struct archive_entry *ae;

    if (archive_read_open_filename (a, archive_path, 10240) != ARCHIVE_OK) {
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED,
                     "Cannot open archive: %s", archive_error_string (a));
        archive_read_free (a);
        return NULL;
    }

    GPtrArray *entries = g_ptr_array_new_with_free_func (g_object_unref);

    while (archive_read_next_header (a, &ae) == ARCHIVE_OK) {
        const char *path = archive_entry_pathname (ae);
        if (!path || path[0] == '\0') continue;

        VarEntryType etype = VAR_ENTRY_FILE;
        mode_t ftype = archive_entry_filetype (ae);
        if (S_ISDIR (ftype))           etype = VAR_ENTRY_DIRECTORY;
        else if (S_ISLNK (ftype))      etype = VAR_ENTRY_SYMLINK;

        gint64 size       = (gint64)archive_entry_size (ae);
        gint64 compressed = -1; /* libarchive doesn't expose per-entry compressed size easily */
        gint64 mtime      = (gint64)archive_entry_mtime (ae);
        guint32 perms     = (guint32)archive_entry_perm (ae);
        gboolean encrypted = archive_entry_is_encrypted (ae);

        VarEntry *entry = var_entry_new_full (path, etype, size, compressed,
                                              mtime, perms, encrypted);
        g_ptr_array_add (entries, entry);

        archive_read_data_skip (a);
    }

    archive_read_free (a);
    return entries;
}

/* ═══════════════════════════════════════════════════
   Extract
   ═══════════════════════════════════════════════════ */

static gboolean
la_extract (VarBackend *backend, const char *archive_path, const char *dest_dir,
            GPtrArray *files_to_extract, const char *password,
            VarOverwritePolicy overwrite, VarProgressCallback progress_cb,
            gpointer progress_data, GCancellable *cancellable, GError **error)
{
    (void)backend;

    struct archive *a = create_reader (password);
    struct archive *ext = archive_write_disk_new ();
    struct archive_entry *ae;

    int flags = ARCHIVE_EXTRACT_TIME | ARCHIVE_EXTRACT_PERM | ARCHIVE_EXTRACT_ACL
              | ARCHIVE_EXTRACT_FFLAGS;

    if (overwrite == VAR_OVERWRITE_ALWAYS)
        flags |= ARCHIVE_EXTRACT_UNLINK;

    archive_write_disk_set_options (ext, flags);
    archive_write_disk_set_standard_lookup (ext);

    if (archive_read_open_filename (a, archive_path, 10240) != ARCHIVE_OK) {
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED,
                     "Cannot open: %s", archive_error_string (a));
        archive_read_free (a);
        archive_write_free (ext);
        return FALSE;
    }

    /* Build a hash set of paths to extract if partial extraction */
    GHashTable *extract_set = NULL;
    if (files_to_extract && files_to_extract->len > 0) {
        extract_set = g_hash_table_new (g_str_hash, g_str_equal);
        for (guint i = 0; i < files_to_extract->len; i++)
            g_hash_table_add (extract_set, g_ptr_array_index (files_to_extract, i));
    }

    gint64 total_entries = 0;
    gint64 extracted = 0;
    gboolean ok = TRUE;

    /* First pass: count total entries for progress */
    {
        struct archive *counter = create_reader (password);
        struct archive_entry *ce;
        archive_read_open_filename (counter, archive_path, 10240);
        while (archive_read_next_header (counter, &ce) == ARCHIVE_OK) {
            total_entries++;
            archive_read_data_skip (counter);
        }
        archive_read_free (counter);
    }

    while (archive_read_next_header (a, &ae) == ARCHIVE_OK) {
        if (cancellable && g_cancellable_is_cancelled (cancellable)) {
            g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_CANCELLED,
                                 "Extraction cancelled");
            ok = FALSE;
            break;
        }

        const char *name = archive_entry_pathname (ae);

        /* Filter if partial extraction */
        if (extract_set && !g_hash_table_contains (extract_set, name)) {
            archive_read_data_skip (a);
            continue;
        }

        /* Redirect entry to dest_dir */
        g_autofree char *full_path = g_build_filename (dest_dir, name, NULL);
        archive_entry_set_pathname (ae, full_path);

        /* Skip existing if policy says so */
        if (overwrite == VAR_OVERWRITE_SKIP) {
            if (g_file_test (full_path, G_FILE_TEST_EXISTS)) {
                archive_read_data_skip (a);
                extracted++;
                continue;
            }
        }

        int r = archive_write_header (ext, ae);
        if (r == ARCHIVE_OK && archive_entry_size (ae) > 0) {
            const void *buff;
            size_t bsize;
            la_int64_t offset;
            while ((r = archive_read_data_block (a, &buff, &bsize, &offset)) == ARCHIVE_OK) {
                archive_write_data_block (ext, buff, bsize, offset);
            }
        }
        archive_write_finish_entry (ext);

        extracted++;
        if (progress_cb) {
            double frac = total_entries > 0 ? (double)extracted / total_entries : 0.0;
            progress_cb (frac, name, extracted, total_entries, progress_data);
        }
    }

    if (extract_set)
        g_hash_table_unref (extract_set);

    archive_read_free (a);
    archive_write_free (ext);
    return ok;
}

/* ═══════════════════════════════════════════════════
   Create
   ═══════════════════════════════════════════════════ */

static int
format_to_archive_format (VarFormatType fmt)
{
    switch (fmt) {
    case VAR_FORMAT_ZIP: case VAR_FORMAT_JAR: case VAR_FORMAT_WAR:
    case VAR_FORMAT_EAR: case VAR_FORMAT_CBZ:
        return ARCHIVE_FORMAT_ZIP;
    case VAR_FORMAT_SEVENZ:
        return ARCHIVE_FORMAT_7ZIP;
    case VAR_FORMAT_CPIO:
        return ARCHIVE_FORMAT_CPIO_POSIX;
    case VAR_FORMAT_AR:
        return ARCHIVE_FORMAT_AR_GNU;
    default:
        return ARCHIVE_FORMAT_TAR_PAX_RESTRICTED;
    }
}

static int
format_to_archive_filter (VarFormatType fmt)
{
    switch (fmt) {
    case VAR_FORMAT_TAR_GZ:   case VAR_FORMAT_GZ:   return ARCHIVE_FILTER_GZIP;
    case VAR_FORMAT_TAR_BZ2:  case VAR_FORMAT_BZ2:  return ARCHIVE_FILTER_BZIP2;
    case VAR_FORMAT_TAR_BZ:   case VAR_FORMAT_BZ:   return ARCHIVE_FILTER_BZIP2;
    case VAR_FORMAT_TAR_XZ:   case VAR_FORMAT_XZ:   return ARCHIVE_FILTER_XZ;
    case VAR_FORMAT_TAR_ZST:  case VAR_FORMAT_ZST:  return ARCHIVE_FILTER_ZSTD;
    case VAR_FORMAT_TAR_LZ:   case VAR_FORMAT_LZ:   return ARCHIVE_FILTER_LZIP;
    case VAR_FORMAT_TAR_Z:    case VAR_FORMAT_Z:     return ARCHIVE_FILTER_COMPRESS;
    default:                                         return ARCHIVE_FILTER_NONE;
    }
}

static gboolean
add_file_to_archive (struct archive *a, const char *filepath,
                     const char *entry_name, GError **error)
{
    struct stat st;
    if (stat (filepath, &st) != 0) {
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_NOT_FOUND,
                     "Cannot stat %s: %s", filepath, strerror (errno));
        return FALSE;
    }

    struct archive_entry *ae = archive_entry_new ();
    archive_entry_set_pathname (ae, entry_name);
    archive_entry_set_size (ae, st.st_size);
    archive_entry_set_filetype (ae, S_ISDIR (st.st_mode) ? AE_IFDIR : AE_IFREG);
    archive_entry_set_perm (ae, st.st_mode & 0777);
    archive_entry_set_mtime (ae, st.st_mtime, 0);

    archive_write_header (a, ae);

    if (!S_ISDIR (st.st_mode)) {
        FILE *f = fopen (filepath, "rb");
        if (f) {
            char buf[8192];
            size_t n;
            while ((n = fread (buf, 1, sizeof(buf), f)) > 0)
                archive_write_data (a, buf, n);
            fclose (f);
        }
    }

    archive_entry_free (ae);
    return TRUE;
}

static void
add_directory_recursive (struct archive *a, const char *dirpath,
                         const char *prefix,
                         VarProgressCallback progress_cb, gpointer progress_data,
                         gint64 *count, gint64 total,
                         GCancellable *cancellable)
{
    GDir *dir = g_dir_open (dirpath, 0, NULL);
    if (!dir) return;

    const char *name;
    while ((name = g_dir_read_name (dir))) {
        if (cancellable && g_cancellable_is_cancelled (cancellable)) break;

        g_autofree char *full = g_build_filename (dirpath, name, NULL);
        g_autofree char *entry_name = prefix
            ? g_build_filename (prefix, name, NULL)
            : g_strdup (name);

        add_file_to_archive (a, full, entry_name, NULL);
        (*count)++;

        if (progress_cb) {
            double frac = total > 0 ? (double)(*count) / total : 0.0;
            progress_cb (frac, entry_name, *count, total, progress_data);
        }

        if (g_file_test (full, G_FILE_TEST_IS_DIR)) {
            add_directory_recursive (a, full, entry_name, progress_cb,
                                     progress_data, count, total, cancellable);
        }
    }
    g_dir_close (dir);
}

static gboolean
la_create (VarBackend *backend, const char *archive_path, VarFormatType format,
           GPtrArray *files, const char *base_dir, const char *password,
           VarCompressionLevel level, VarProgressCallback progress_cb,
           gpointer progress_data, GCancellable *cancellable, GError **error)
{
    (void)backend;

    struct archive *a = archive_write_new ();

    int afmt = format_to_archive_format (format);
    int afilt = format_to_archive_filter (format);

    archive_write_set_format (a, afmt);
    if (afilt != ARCHIVE_FILTER_NONE)
        archive_write_add_filter (a, afilt);
    else if (afmt == ARCHIVE_FORMAT_ZIP || afmt == ARCHIVE_FORMAT_7ZIP)
        archive_write_add_filter_none (a);
    else
        archive_write_add_filter_none (a);

    /* Compression level */
    if (level != VAR_COMPRESS_STORE) {
        char lvl_str[4];
        g_snprintf (lvl_str, sizeof(lvl_str), "%d", (int)level);
        archive_write_set_filter_option (a, NULL, "compression-level", lvl_str);
    }

    /* Password */
    if (password && password[0]) {
        archive_write_set_passphrase (a, password);
        if (afmt == ARCHIVE_FORMAT_ZIP) {
            archive_write_set_options (a, "zip:encryption=aes256");
        }
    }

    if (archive_write_open_filename (a, archive_path) != ARCHIVE_OK) {
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED,
                     "Cannot create archive: %s", archive_error_string (a));
        archive_write_free (a);
        return FALSE;
    }

    gint64 count = 0;
    gint64 total = files ? (gint64)files->len : 0;

    if (files) {
        for (guint i = 0; i < files->len; i++) {
            if (cancellable && g_cancellable_is_cancelled (cancellable)) break;

            const char *filepath = (const char *)g_ptr_array_index (files, i);
            g_autofree char *entry_name = NULL;

            if (base_dir) {
                /* Make relative to base_dir */
                if (g_str_has_prefix (filepath, base_dir)) {
                    const char *rel = filepath + strlen (base_dir);
                    while (*rel == '/') rel++;
                    entry_name = g_strdup (rel);
                } else {
                    entry_name = g_path_get_basename (filepath);
                }
            } else {
                entry_name = g_path_get_basename (filepath);
            }

            add_file_to_archive (a, filepath, entry_name, error);
            count++;

            if (g_file_test (filepath, G_FILE_TEST_IS_DIR)) {
                add_directory_recursive (a, filepath, entry_name,
                                         progress_cb, progress_data,
                                         &count, total, cancellable);
            }

            if (progress_cb) {
                double frac = total > 0 ? (double)count / total : 0.0;
                progress_cb (frac, entry_name, count, total, progress_data);
            }
        }
    }

    archive_write_close (a);
    archive_write_free (a);
    return TRUE;
}

/* ═══════════════════════════════════════════════════
   Test
   ═══════════════════════════════════════════════════ */

static gboolean
la_test (VarBackend *backend, const char *archive_path,
         const char *password, GError **error)
{
    (void)backend;

    struct archive *a = create_reader (password);
    struct archive_entry *ae;

    if (archive_read_open_filename (a, archive_path, 10240) != ARCHIVE_OK) {
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED,
                     "Test failed: %s", archive_error_string (a));
        archive_read_free (a);
        return FALSE;
    }

    gboolean ok = TRUE;
    while (archive_read_next_header (a, &ae) == ARCHIVE_OK) {
        /* Read all data blocks to verify integrity */
        const void *buff;
        size_t bsize;
        la_int64_t offset;
        int r;
        while ((r = archive_read_data_block (a, &buff, &bsize, &offset)) == ARCHIVE_OK);
        if (r != ARCHIVE_EOF) {
            g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED,
                         "Integrity error in '%s': %s",
                         archive_entry_pathname (ae),
                         archive_error_string (a));
            ok = FALSE;
            break;
        }
    }

    archive_read_free (a);
    return ok;
}

/* ═══════════════════════════════════════════════════
   Stubs for add/delete (re-pack approach)
   ═══════════════════════════════════════════════════ */

static gboolean
la_add (VarBackend *backend, const char *archive_path, GPtrArray *files,
        const char *base_dir, const char *password,
        GCancellable *cancel, GError **error)
{
    (void)backend; (void)cancel;

    /* For formats that support appending (zip), we re-create.
       This is a simplified approach: read existing + add new. */

    /* Step 1: List existing entries */
    GPtrArray *existing = la_list (backend, archive_path, password, error);
    if (!existing) return FALSE;

    /* Step 2: Detect format */
    VarFormatType fmt = var_utils_detect_format (archive_path);

    /* Step 3: Create temp archive with existing + new files */
    g_autofree char *tmppath = g_strdup_printf ("%s.vartmp", archive_path);

    /* For now, create a new archive with the files */
    gboolean ok = la_create (backend, tmppath, fmt, files, base_dir, password,
                              VAR_COMPRESS_NORMAL, NULL, NULL, cancel, error);

    g_ptr_array_unref (existing);

    if (ok) {
        /* Replace original with temp */
        g_rename (tmppath, archive_path);
    } else {
        g_unlink (tmppath);
    }

    return ok;
}

static gboolean
la_delete (VarBackend *backend, const char *archive_path, GPtrArray *entry_paths,
           GCancellable *cancel, GError **error)
{
    (void)backend; (void)cancel;

    /* Re-create archive without the specified entries */
    struct archive *a = create_reader (NULL);
    struct archive_entry *ae;

    if (archive_read_open_filename (a, archive_path, 10240) != ARCHIVE_OK) {
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED,
                     "Cannot open: %s", archive_error_string (a));
        archive_read_free (a);
        return FALSE;
    }

    GHashTable *delete_set = g_hash_table_new (g_str_hash, g_str_equal);
    for (guint i = 0; i < entry_paths->len; i++)
        g_hash_table_add (delete_set, g_ptr_array_index (entry_paths, i));

    VarFormatType fmt = var_utils_detect_format (archive_path);
    g_autofree char *tmppath = g_strdup_printf ("%s.vartmp", archive_path);

    struct archive *w = archive_write_new ();
    archive_write_set_format (w, format_to_archive_format (fmt));
    int afilt = format_to_archive_filter (fmt);
    if (afilt != ARCHIVE_FILTER_NONE)
        archive_write_add_filter (w, afilt);
    else
        archive_write_add_filter_none (w);

    archive_write_open_filename (w, tmppath);

    while (archive_read_next_header (a, &ae) == ARCHIVE_OK) {
        const char *name = archive_entry_pathname (ae);
        if (g_hash_table_contains (delete_set, name)) {
            archive_read_data_skip (a);
            continue;
        }

        archive_write_header (w, ae);
        if (archive_entry_size (ae) > 0) {
            const void *buff;
            size_t bsize;
            la_int64_t offset;
            while (archive_read_data_block (a, &buff, &bsize, &offset) == ARCHIVE_OK)
                archive_write_data_block (w, buff, bsize, offset);
        }
        archive_write_finish_entry (w);
    }

    archive_write_close (w);
    archive_write_free (w);
    archive_read_free (a);
    g_hash_table_unref (delete_set);

    g_rename (tmppath, archive_path);
    return TRUE;
}

/* ═══════════════════════════════════════════════════
   Interface Init
   ═══════════════════════════════════════════════════ */

static void
var_libarchive_backend_iface_init (VarBackendInterface *iface)
{
    iface->list           = la_list;
    iface->extract        = la_extract;
    iface->create         = la_create;
    iface->add            = la_add;
    iface->delete_entries = la_delete;
    iface->test           = la_test;
}

static void
var_libarchive_backend_class_init (VarLibarchiveBackendClass *klass)
{
    (void)klass;
}

static void
var_libarchive_backend_init (VarLibarchiveBackend *self)
{
    (void)self;
}

VarLibarchiveBackend *
var_libarchive_backend_new (void)
{
    return g_object_new (VAR_TYPE_LIBARCHIVE_BACKEND, NULL);
}
