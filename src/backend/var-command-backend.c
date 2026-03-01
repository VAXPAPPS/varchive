/*
 * VArchive - Professional Archive Manager for VAXP-OS
 * Copyright (C) 2026 VAXP Organization
 *
 * var-command-backend.c — CLI tool fallback backend
 *
 * Handles formats not natively supported by libarchive by
 * shelling out to external CLI tools (brotli, bzip3, lrzip,
 * lzop, rzip, arj, zoo, unace, unalz, unar, unsquashfs, etc.)
 */

#include "var-command-backend.h"
#include "../core/var-utils.h"
#include <string.h>
#include <glib/gstdio.h>

struct _VarCommandBackend {
    GObject  parent_instance;
};

static void var_command_backend_iface_init (VarBackendInterface *iface);

G_DEFINE_TYPE_WITH_CODE (VarCommandBackend, var_command_backend, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE (VAR_TYPE_BACKEND, var_command_backend_iface_init))

/* ═══════════════════════════════════════════════════
   Tool availability check
   ═══════════════════════════════════════════════════ */

gboolean
var_command_backend_tool_available (const char *tool_name)
{
    g_autofree char *path = g_find_program_in_path (tool_name);
    return path != NULL;
}

/* ═══════════════════════════════════════════════════
   Helper: determine which tool handles a format
   ═══════════════════════════════════════════════════ */

typedef struct {
    const char *tool;
    const char *list_args;
    const char *extract_args;
    const char *create_args;     /* NULL = read-only */
} ToolInfo;

static const ToolInfo *
get_tool_for_format (VarFormatType format)
{
    static const ToolInfo tools[] = {
        [0] = { "brotli",     NULL,                "-d -o %o %i",         "-o %o %i" },
        [1] = { "bzip3",      NULL,                "-d -k %i",            "-k %i" },
        [2] = { "lrzip",      NULL,                "-d -o %o %i",         "-L %l -o %o %i" },
        [3] = { "lzop",       "--ls %i",           "-d -o %o %i",         "-%l -o %o %i" },
        [4] = { "rzip",       NULL,                "-d -o %o %i",         "-k -%l -o %o %i" },
        [5] = { "arj",        "l %i",              "x %i %o",            "a %i %f" },
        [6] = { "zoo",        "l %i",              "x %i",               "a %i %f" },
        [7] = { "unace",      "l %i",              "x %i %o",            NULL },
        [8] = { "unalz",      "-l %i",             "%i -d %o",           NULL },
        [9] = { "unar",       "-l %i",             "-o %o %i",           NULL },
        [10] = { "unsquashfs", "-l %i",            "-d %o %i",           NULL },
    };

    switch (format) {
    case VAR_FORMAT_BR:     case VAR_FORMAT_TAR_BR:  return &tools[0];
    case VAR_FORMAT_BZ3:    case VAR_FORMAT_TAR_BZ3: return &tools[1];
    case VAR_FORMAT_LRZ:    case VAR_FORMAT_TAR_LRZ: return &tools[2];
    case VAR_FORMAT_LZOP:   case VAR_FORMAT_TAR_LZOP: return &tools[3];
    case VAR_FORMAT_RZ:                              return &tools[4];
    case VAR_FORMAT_ARJ:                             return &tools[5];
    case VAR_FORMAT_ZOO:                             return &tools[6];
    case VAR_FORMAT_ACE:                             return &tools[7];
    case VAR_FORMAT_ALZ:                             return &tools[8];
    case VAR_FORMAT_STUFFIT:                         return &tools[9];
    case VAR_FORMAT_DMG:                             return &tools[9]; /* unar handles dmg */
    case VAR_FORMAT_SNAP:   case VAR_FORMAT_SQUASHFS: return &tools[10];
    default:                                          return NULL;
    }
}

/* ═══════════════════════════════════════════════════
   Helper: Run a command synchronously
   ═══════════════════════════════════════════════════ */

static gboolean
run_tool (const char *cmdline, char **stdout_out, GError **error)
{
    gint exit_status;
    char *stderr_out = NULL;
    char *stdout_data = NULL;

    gboolean ok = g_spawn_command_line_sync (cmdline,
                                              &stdout_data,
                                              &stderr_out,
                                              &exit_status,
                                              error);
    g_free (stderr_out);

    if (ok && stdout_out)
        *stdout_out = stdout_data;
    else
        g_free (stdout_data);

    if (ok && !g_spawn_check_wait_status (exit_status, error))
        ok = FALSE;

    return ok;
}

/* ═══════════════════════════════════════════════════
   Two-step decompression for tar+compressor combos
   ═══════════════════════════════════════════════════ */

static gboolean
is_tar_plus_compressor (VarFormatType fmt)
{
    switch (fmt) {
    case VAR_FORMAT_TAR_BR:
    case VAR_FORMAT_TAR_BZ3:
    case VAR_FORMAT_TAR_LRZ:
    case VAR_FORMAT_TAR_LZOP:
        return TRUE;
    default:
        return FALSE;
    }
}

/* ═══════════════════════════════════════════════════
   List implementation
   ═══════════════════════════════════════════════════ */

static GPtrArray *
cmd_list (VarBackend *backend, const char *archive_path,
          const char *password, GError **error)
{
    (void)backend; (void)password;

    VarFormatType fmt = var_utils_detect_format (archive_path);
    const ToolInfo *ti = get_tool_for_format (fmt);

    if (!ti) {
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED,
                     "No tool available for this format");
        return NULL;
    }

    if (!var_command_backend_tool_available (ti->tool)) {
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED,
                     "Required tool '%s' is not installed", ti->tool);
        return NULL;
    }

    /* For single-file compressors, just return one entry */
    if (var_utils_format_is_single_file (fmt)) {
        GPtrArray *entries = g_ptr_array_new_with_free_func (g_object_unref);
        /* Strip the compression extension to get the inner filename */
        g_autofree char *basename = g_path_get_basename (archive_path);
        const char *ext = var_utils_format_extension (fmt);
        size_t blen = strlen (basename);
        size_t elen = strlen (ext);
        if (blen > elen)
            basename[blen - elen] = '\0';

        VarEntry *entry = var_entry_new_full (basename, VAR_ENTRY_FILE, -1, -1, 0, 0644, FALSE);
        g_ptr_array_add (entries, entry);
        return entries;
    }

    /* Run list command */
    if (ti->list_args) {
        g_autofree char *cmd = g_strdup_printf ("%s %s",
            ti->tool,
            g_strdup_printf ("%s", ti->list_args));

        /* Replace %i with archive path */
        g_autofree char *quoted = g_shell_quote (archive_path);
        g_autofree char *final_cmd = NULL;
        {
            GString *s = g_string_new (NULL);
            g_string_printf (s, "%s", ti->tool);
            g_autofree char *args = g_strdup (ti->list_args);
            char *pi = strstr (args, "%i");
            if (pi) {
                *pi = '\0';
                g_string_append_printf (s, " %s%s%s", args, quoted, pi + 2);
            } else {
                g_string_append_printf (s, " %s %s", args, quoted);
            }
            final_cmd = g_string_free (s, FALSE);
        }

        char *output = NULL;
        if (!run_tool (final_cmd, &output, error)) {
            g_free (output);
            return NULL;
        }

        /* Parse output lines as filenames (simplified) */
        GPtrArray *entries = g_ptr_array_new_with_free_func (g_object_unref);
        if (output) {
            char **lines = g_strsplit (output, "\n", -1);
            for (int i = 0; lines[i]; i++) {
                g_strstrip (lines[i]);
                if (lines[i][0] == '\0') continue;
                /* Each line could be a filename or formatted output */
                VarEntry *entry = var_entry_new_full (lines[i], VAR_ENTRY_FILE,
                                                      -1, -1, 0, 0, FALSE);
                g_ptr_array_add (entries, entry);
            }
            g_strfreev (lines);
        }
        g_free (output);
        return entries;
    }

    /* No list command available — return empty */
    return g_ptr_array_new_with_free_func (g_object_unref);
}

/* ═══════════════════════════════════════════════════
   Extract implementation
   ═══════════════════════════════════════════════════ */

static gboolean
cmd_extract (VarBackend *backend, const char *archive_path, const char *dest_dir,
             GPtrArray *files_to_extract, const char *password,
             VarOverwritePolicy overwrite, VarProgressCallback progress_cb,
             gpointer progress_data, GCancellable *cancellable, GError **error)
{
    (void)backend; (void)files_to_extract; (void)password;
    (void)overwrite; (void)cancellable;

    VarFormatType fmt = var_utils_detect_format (archive_path);
    const ToolInfo *ti = get_tool_for_format (fmt);

    if (!ti || !var_command_backend_tool_available (ti->tool)) {
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED,
                     "Required tool '%s' is not installed", ti->tool);
        return FALSE;
    }

    g_autofree char *quoted_input = g_shell_quote (archive_path);
    g_autofree char *quoted_output = g_shell_quote (dest_dir);

    /* For tar+compressor: decompress to temp tar, then extract tar */
    if (is_tar_plus_compressor (fmt)) {
        /* Step 1: Decompress to temp .tar */
        g_autofree char *basename = g_path_get_basename (archive_path);
        g_autofree char *tmptar = g_build_filename (dest_dir, "varchive_temp.tar", NULL);
        g_autofree char *quoted_tmp = g_shell_quote (tmptar);

        GString *cmd1 = g_string_new (NULL);
        g_string_printf (cmd1, "%s", ti->tool);

        if (fmt == VAR_FORMAT_TAR_BR)
            g_string_append_printf (cmd1, " -d -o %s %s", quoted_tmp, quoted_input);
        else if (fmt == VAR_FORMAT_TAR_BZ3)
            g_string_append_printf (cmd1, " -d -k -o %s %s", quoted_tmp, quoted_input);
        else if (fmt == VAR_FORMAT_TAR_LRZ)
            g_string_append_printf (cmd1, " -d -o %s %s", quoted_tmp, quoted_input);
        else if (fmt == VAR_FORMAT_TAR_LZOP)
            g_string_append_printf (cmd1, " -d -p %s -o %s", quoted_input, quoted_tmp);

        g_autofree char *cmd1_str = g_string_free (cmd1, FALSE);

        if (progress_cb)
            progress_cb (0.3, "Decompressing...", 0, 0, progress_data);

        if (!run_tool (cmd1_str, NULL, error)) {
            g_unlink (tmptar);
            return FALSE;
        }

        /* Step 2: Extract tar using libarchive or tar command */
        g_autofree char *tar_cmd = g_strdup_printf ("tar xf %s -C %s", quoted_tmp, quoted_output);

        if (progress_cb)
            progress_cb (0.7, "Extracting tar...", 0, 0, progress_data);

        gboolean ok = run_tool (tar_cmd, NULL, error);
        g_unlink (tmptar);

        if (progress_cb)
            progress_cb (1.0, "Done", 0, 0, progress_data);

        return ok;
    }

    /* Single-file compressor: decompress to dest */
    if (var_utils_format_is_single_file (fmt)) {
        g_autofree char *basename = g_path_get_basename (archive_path);
        const char *ext = var_utils_format_extension (fmt);
        size_t blen = strlen (basename);
        size_t elen = strlen (ext);
        if (blen > elen) basename[blen - elen] = '\0';

        g_autofree char *outfile = g_build_filename (dest_dir, basename, NULL);
        g_autofree char *quoted_out = g_shell_quote (outfile);

        GString *cmd = g_string_new (NULL);

        if (fmt == VAR_FORMAT_BR)
            g_string_printf (cmd, "brotli -d -o %s %s", quoted_out, quoted_input);
        else if (fmt == VAR_FORMAT_BZ3)
            g_string_printf (cmd, "bzip3 -d -k %s", quoted_input);
        else if (fmt == VAR_FORMAT_LRZ)
            g_string_printf (cmd, "lrzip -d -o %s %s", quoted_out, quoted_input);
        else if (fmt == VAR_FORMAT_LZOP)
            g_string_printf (cmd, "lzop -d -o %s %s", quoted_out, quoted_input);
        else if (fmt == VAR_FORMAT_RZ)
            g_string_printf (cmd, "rzip -d -k -o %s %s", quoted_out, quoted_input);
        else
            g_string_printf (cmd, "%s -d %s", ti->tool, quoted_input);

        g_autofree char *cmd_str = g_string_free (cmd, FALSE);

        if (progress_cb)
            progress_cb (0.5, basename, 0, 0, progress_data);

        gboolean ok = run_tool (cmd_str, NULL, error);

        if (progress_cb)
            progress_cb (1.0, "Done", 0, 0, progress_data);

        return ok;
    }

    /* Multi-file archive tools (arj, zoo, unace, unalz, unar, unsquashfs) */
    GString *cmd = g_string_new (NULL);

    if (fmt == VAR_FORMAT_ARJ)
        g_string_printf (cmd, "arj x %s %s/", quoted_input, quoted_output);
    else if (fmt == VAR_FORMAT_ZOO)
        g_string_printf (cmd, "cd %s && zoo x %s", quoted_output, quoted_input);
    else if (fmt == VAR_FORMAT_ACE)
        g_string_printf (cmd, "unace x %s %s/", quoted_input, quoted_output);
    else if (fmt == VAR_FORMAT_ALZ)
        g_string_printf (cmd, "unalz %s -d %s", quoted_input, quoted_output);
    else if (fmt == VAR_FORMAT_STUFFIT || fmt == VAR_FORMAT_DMG)
        g_string_printf (cmd, "unar -o %s %s", quoted_output, quoted_input);
    else if (fmt == VAR_FORMAT_SNAP || fmt == VAR_FORMAT_SQUASHFS)
        g_string_printf (cmd, "unsquashfs -d %s/squashfs-root %s", quoted_output, quoted_input);
    else
        g_string_printf (cmd, "%s x %s", ti->tool, quoted_input);

    g_autofree char *cmd_str = g_string_free (cmd, FALSE);

    if (progress_cb)
        progress_cb (0.5, "Extracting...", 0, 0, progress_data);

    gboolean ok = run_tool (cmd_str, NULL, error);

    if (progress_cb)
        progress_cb (1.0, "Done", 0, 0, progress_data);

    return ok;
}

/* ═══════════════════════════════════════════════════
   Create implementation
   ═══════════════════════════════════════════════════ */

static gboolean
cmd_create (VarBackend *backend, const char *archive_path, VarFormatType format,
            GPtrArray *files, const char *base_dir, const char *password,
            VarCompressionLevel level, VarProgressCallback progress_cb,
            gpointer progress_data, GCancellable *cancellable, GError **error)
{
    (void)backend; (void)password; (void)cancellable;

    const ToolInfo *ti = get_tool_for_format (format);
    if (!ti || !ti->create_args) {
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED,
                     "This format does not support creation");
        return FALSE;
    }

    if (!var_command_backend_tool_available (ti->tool)) {
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED,
                     "Required tool '%s' is not installed", ti->tool);
        return FALSE;
    }

    /* For tar+compressor: create tar first, then compress */
    if (is_tar_plus_compressor (format)) {
        /* Build tar command */
        GString *tar_cmd = g_string_new ("tar cf ");
        g_autofree char *tmptar = g_strdup_printf ("%s.tar", archive_path);
        g_autofree char *quoted_tmp = g_shell_quote (tmptar);
        g_string_append (tar_cmd, quoted_tmp);

        if (base_dir) {
            g_autofree char *qdir = g_shell_quote (base_dir);
            g_string_append_printf (tar_cmd, " -C %s", qdir);
        }

        if (files) {
            for (guint i = 0; i < files->len; i++) {
                GFile *f = G_FILE (g_ptr_array_index (files, i));
                g_autofree char *path = g_file_get_path (f);
                g_autofree char *qp = g_shell_quote (path);
                g_string_append_printf (tar_cmd, " %s", qp);
            }
        }

        g_autofree char *tar_str = g_string_free (tar_cmd, FALSE);

        if (progress_cb)
            progress_cb (0.3, "Creating tar...", 0, 0, progress_data);

        if (!run_tool (tar_str, NULL, error)) {
            g_unlink (tmptar);
            return FALSE;
        }

        /* Compress the tar */
        g_autofree char *quoted_out = g_shell_quote (archive_path);
        GString *comp_cmd = g_string_new (NULL);

        if (format == VAR_FORMAT_TAR_BR)
            g_string_printf (comp_cmd, "brotli -o %s %s", quoted_out, quoted_tmp);
        else if (format == VAR_FORMAT_TAR_BZ3)
            g_string_printf (comp_cmd, "bzip3 -k %s", quoted_tmp);
        else if (format == VAR_FORMAT_TAR_LRZ)
            g_string_printf (comp_cmd, "lrzip -L %d -o %s %s", (int)level, quoted_out, quoted_tmp);
        else if (format == VAR_FORMAT_TAR_LZOP)
            g_string_printf (comp_cmd, "lzop -%d -o %s %s", (int)level, quoted_out, quoted_tmp);

        g_autofree char *comp_str = g_string_free (comp_cmd, FALSE);

        if (progress_cb)
            progress_cb (0.7, "Compressing...", 0, 0, progress_data);

        gboolean ok = run_tool (comp_str, NULL, error);
        g_unlink (tmptar);

        if (progress_cb)
            progress_cb (1.0, "Done", 0, 0, progress_data);

        return ok;
    }

    /* Single-file compressor */
    if (var_utils_format_is_single_file (format) && files && files->len == 1) {
        GFile *f = G_FILE (g_ptr_array_index (files, 0));
        g_autofree char *filepath = g_file_get_path (f);
        g_autofree char *quoted_in = g_shell_quote (filepath);
        g_autofree char *quoted_out = g_shell_quote (archive_path);

        GString *cmd = g_string_new (NULL);

        if (format == VAR_FORMAT_BR)
            g_string_printf (cmd, "brotli -o %s %s", quoted_out, quoted_in);
        else if (format == VAR_FORMAT_BZ3)
            g_string_printf (cmd, "bzip3 -k %s", quoted_in);
        else if (format == VAR_FORMAT_LRZ)
            g_string_printf (cmd, "lrzip -L %d -o %s %s", (int)level, quoted_out, quoted_in);
        else if (format == VAR_FORMAT_LZOP)
            g_string_printf (cmd, "lzop -%d -o %s %s", (int)level, quoted_out, quoted_in);
        else if (format == VAR_FORMAT_RZ)
            g_string_printf (cmd, "rzip -k -%d -o %s %s", (int)level, quoted_out, quoted_in);

        g_autofree char *cmd_str = g_string_free (cmd, FALSE);
        return run_tool (cmd_str, NULL, error);
    }

    /* Multi-file: arj, zoo */
    GString *cmd = g_string_new (NULL);
    g_autofree char *quoted_archive = g_shell_quote (archive_path);

    if (format == VAR_FORMAT_ARJ)
        g_string_printf (cmd, "arj a %s", quoted_archive);
    else if (format == VAR_FORMAT_ZOO)
        g_string_printf (cmd, "zoo a %s", quoted_archive);
    else
        g_string_printf (cmd, "%s a %s", ti->tool, quoted_archive);

    if (files) {
        for (guint i = 0; i < files->len; i++) {
            GFile *f = G_FILE (g_ptr_array_index (files, i));
            g_autofree char *path = g_file_get_path (f);
            g_autofree char *qp = g_shell_quote (path);
            g_string_append_printf (cmd, " %s", qp);
        }
    }

    g_autofree char *cmd_str = g_string_free (cmd, FALSE);
    return run_tool (cmd_str, NULL, error);
}

/* ═══════════════════════════════════════════════════
   Stubs
   ═══════════════════════════════════════════════════ */

static gboolean
cmd_add (VarBackend *b, const char *a, GPtrArray *f, const char *d,
         const char *p, GCancellable *c, GError **e)
{
    (void)b; (void)a; (void)f; (void)d; (void)p; (void)c;
    g_set_error_literal (e, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED,
                         "Add not supported for this format");
    return FALSE;
}

static gboolean
cmd_delete (VarBackend *b, const char *a, GPtrArray *p,
            GCancellable *c, GError **e)
{
    (void)b; (void)a; (void)p; (void)c;
    g_set_error_literal (e, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED,
                         "Delete not supported for this format");
    return FALSE;
}

static gboolean
cmd_test (VarBackend *b, const char *a, const char *p, GError **e)
{
    (void)b; (void)a; (void)p;
    g_set_error_literal (e, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED,
                         "Test not supported for this format via CLI tools");
    return FALSE;
}

/* ═══════════════════════════════════════════════════
   Interface Init
   ═══════════════════════════════════════════════════ */

static void
var_command_backend_iface_init (VarBackendInterface *iface)
{
    iface->list           = cmd_list;
    iface->extract        = cmd_extract;
    iface->create         = cmd_create;
    iface->add            = cmd_add;
    iface->delete_entries = cmd_delete;
    iface->test           = cmd_test;
}

static void
var_command_backend_class_init (VarCommandBackendClass *klass)
{
    (void)klass;
}

static void
var_command_backend_init (VarCommandBackend *self)
{
    (void)self;
}

VarCommandBackend *
var_command_backend_new (void)
{
    return g_object_new (VAR_TYPE_COMMAND_BACKEND, NULL);
}
