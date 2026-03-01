/*
 * VArchive - Professional Archive Manager for VAXP-OS
 * Copyright (C) 2026 VAXP Organization
 *
 * var-main-window.c — Main application window
 */

#include "var-main-window.h"
#include "../../services/var-archive-service.h"
#include "../widgets/var-headerbar.h"
#include "../widgets/var-toolbar.h"
#include "../widgets/var-location-bar.h"
#include "../widgets/var-archive-view.h"
#include "../widgets/var-progress-overlay.h"
#include "../dialogs/var-extract-dialog.h"
#include "../dialogs/var-create-dialog.h"
#include "../dialogs/var-properties-dialog.h"
#include "../dialogs/var-preferences-dialog.h"
#include "../dialogs/var-password-dialog.h"
#include "../../core/var-utils.h"

struct _VarMainWindow {
    AdwApplicationWindow parent_instance;

    VarHeaderBar       *header_bar;
    VarToolbar         *toolbar;
    VarLocationBar     *location_bar;
    VarArchiveView     *archive_view;
    VarProgressOverlay *progress_overlay;

    GtkWidget          *status_label;
    GtkWidget          *overlay;

    char               *current_archive_path;
    gboolean            is_modified;
};

G_DEFINE_TYPE (VarMainWindow, var_main_window, ADW_TYPE_APPLICATION_WINDOW)

/* ═══════════════════════════════════════════════════
   State updates
   ═══════════════════════════════════════════════════ */

static void
update_window_title (VarMainWindow *self)
{
    if (self->current_archive_path) {
        g_autofree char *basename = g_path_get_basename (self->current_archive_path);
        g_autofree char *title = g_strdup_printf ("%s%s", basename, self->is_modified ? " *" : "");
        var_headerbar_set_title (self->header_bar, title);
        
        VarFormatType fmt = var_utils_detect_format (self->current_archive_path);
        var_headerbar_set_subtitle (self->header_bar, var_utils_format_name (fmt));
    } else {
        var_headerbar_set_title (self->header_bar, VAR_APP_NAME);
        var_headerbar_set_subtitle (self->header_bar, "No archive opened");
    }
}

static void
update_status (VarMainWindow *self, const char *msg)
{
    gtk_label_set_text (GTK_LABEL (self->status_label), msg ? msg : "Ready");
}

/* ═══════════════════════════════════════════════════
   Async Callbacks
   ═══════════════════════════════════════════════════ */

static void
on_archive_opened (GObject *source_object, GAsyncResult *res, gpointer user_data)
{
    VarArchiveService *service = VAR_ARCHIVE_SERVICE (source_object);
    VarMainWindow *self = VAR_MAIN_WINDOW (user_data);
    
    GError *error = NULL;
    GPtrArray *entries = var_archive_service_open_finish (service, res, &error);
    
    var_progress_overlay_hide (self->progress_overlay);

    if (error) {
        GtkWidget *dialog = gtk_message_dialog_new (GTK_WINDOW (self),
            GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
            "Could not open archive: %s", error->message);
        gtk_window_present (GTK_WINDOW (dialog));
        g_signal_connect(dialog, "response", G_CALLBACK(gtk_window_destroy), NULL);
        g_error_free (error);
        return;
    }

    var_archive_view_set_model (self->archive_view, entries);
    
    if (entries) {
        g_autofree char *status = g_strdup_printf ("%u items loaded", entries->len);
        update_status (self, status);
        g_ptr_array_unref (entries);
    }

    var_toolbar_set_archive_opened (self->toolbar, TRUE);
    update_window_title (self);
}

/* ═══════════════════════════════════════════════════
   Actions
   ═══════════════════════════════════════════════════ */

static void
on_open_response (GtkDialog *dialog, int response, gpointer user_data)
{
    VarMainWindow *self = VAR_MAIN_WINDOW (user_data);
    
    if (response == GTK_RESPONSE_ACCEPT) {
        GtkFileChooser *chooser = GTK_FILE_CHOOSER (dialog);
        g_autoptr(GFile) file = gtk_file_chooser_get_file (chooser);
        if (file) {
            var_main_window_open_file (self, file);
        }
    }
    gtk_window_destroy (GTK_WINDOW (dialog));
}

static void
action_open (VarToolbar *toolbar, gpointer user_data)
{
    (void)toolbar;
    VarMainWindow *self = VAR_MAIN_WINDOW (user_data);
    
    GtkWidget *dialog = gtk_file_chooser_dialog_new ("Open Archive",
                                                     GTK_WINDOW (self),
                                                     GTK_FILE_CHOOSER_ACTION_OPEN,
                                                     "Cancel", GTK_RESPONSE_CANCEL,
                                                     "Open", GTK_RESPONSE_ACCEPT,
                                                     NULL);
    
    GtkFileFilter *filter = gtk_file_filter_new ();
    gtk_file_filter_set_name (filter, "All Supported Archives");
    /* Add a few common ones for the filter */
    gtk_file_filter_add_pattern (filter, "*.zip");
    gtk_file_filter_add_pattern (filter, "*.tar.*");
    gtk_file_filter_add_pattern (filter, "*.7z");
    gtk_file_filter_add_pattern (filter, "*.rar");
    gtk_file_filter_add_pattern (filter, "*.iso");
    gtk_file_filter_add_pattern (filter, "*.deb");
    gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dialog), filter);
    
    GtkFileFilter *all = gtk_file_filter_new ();
    gtk_file_filter_set_name (all, "All Files");
    gtk_file_filter_add_pattern (all, "*");
    gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dialog), all);
    
    gtk_window_present (GTK_WINDOW (dialog));
    g_signal_connect (dialog, "response", G_CALLBACK (on_open_response), self);
}

static void
on_extract_progress (VarArchiveService *service, double fraction, const char *file, gpointer user_data)
{
    (void)service;
    VarMainWindow *self = VAR_MAIN_WINDOW (user_data);
    var_progress_overlay_set_fraction (self->progress_overlay, fraction);
    var_progress_overlay_set_detail (self->progress_overlay, file);
}

static void
on_extract_finished (GObject *source_object, GAsyncResult *res, gpointer user_data)
{
    VarArchiveService *service = VAR_ARCHIVE_SERVICE (source_object);
    VarMainWindow *self = VAR_MAIN_WINDOW (user_data);
    
    /* Disconnect progress signal */
    g_signal_handlers_disconnect_by_func (service, on_extract_progress, self);
    
    GError *error = NULL;
    gboolean ok = var_archive_service_extract_finish (service, res, &error);
    
    var_progress_overlay_hide (self->progress_overlay);
    
    if (!ok) {
        GtkWidget *dialog = gtk_message_dialog_new (GTK_WINDOW (self),
            GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
            "Extraction failed: %s", error ? error->message : "Unknown error");
        gtk_window_present (GTK_WINDOW (dialog));
        g_signal_connect(dialog, "response", G_CALLBACK(gtk_window_destroy), NULL);
        if (error) g_error_free (error);
        
        update_status (self, "Extraction failed");
    } else {
        update_status (self, "Extraction completed successfully");
    }
}

static void
on_extract_settings_confirmed (const char *dest_path, gboolean extract_all, gboolean create_folder, VarOverwritePolicy overwrite, gpointer user_data)
{
    VarMainWindow *self = VAR_MAIN_WINDOW (user_data);
    if (!self->current_archive_path) return;
    
    GPtrArray *files_to_extract = NULL;
    if (!extract_all) {
        /* Only extract selected */
        files_to_extract = var_archive_view_get_selected_paths (self->archive_view);
    }
    
    g_autofree char *final_dest = NULL;
    if (create_folder) {
        g_autofree char *basename = g_path_get_basename (self->current_archive_path);
        /* Strip extension if possible, or just use basename */
        VarFormatType fmt = var_utils_detect_format (self->current_archive_path);
        const char *ext = var_utils_format_extension (fmt);
        if (g_str_has_suffix (basename, ext)) {
            basename[strlen(basename) - strlen(ext)] = '\0';
        }
        final_dest = g_build_filename (dest_path, basename, NULL);
        g_mkdir_with_parents (final_dest, 0755);
    } else {
        final_dest = g_strdup (dest_path);
    }
    
    var_progress_overlay_show (self->progress_overlay, "Extracting...");
    
    VarArchiveService *service = var_archive_service_get_default ();
    g_signal_connect (service, "progress-changed", G_CALLBACK (on_extract_progress), self);
    
    var_archive_service_extract_async (service,
        self->current_archive_path,
        final_dest,
        files_to_extract,
        NULL, /* password */
        overwrite,
        NULL, /* cancellable */
        on_extract_finished,
        self);
        
    if (files_to_extract)
        g_ptr_array_unref (files_to_extract);
}

static void
action_extract (VarToolbar *toolbar, gpointer user_data)
{
    (void)toolbar;
    VarMainWindow *self = VAR_MAIN_WINDOW (user_data);
    if (!self->current_archive_path) return;
    
    gboolean has_selection = var_archive_view_has_selection (self->archive_view);
    
    VarExtractDialog *dialog = var_extract_dialog_new (GTK_WINDOW (self), self->current_archive_path, has_selection);
    var_extract_dialog_set_callback (dialog, on_extract_settings_confirmed, self);
    gtk_window_present (GTK_WINDOW (dialog));
}

typedef struct {
    VarMainWindow *window;
    char *dest_path;
    VarFormatType format;
    VarCompressionLevel level;
    char *password;
} CreateContext;

static void
create_context_free (CreateContext *ctx) {
    g_free (ctx->dest_path);
    g_free (ctx->password);
    g_free (ctx);
}

static void
on_create_progress (VarArchiveService *service, double fraction, const char *file, gpointer user_data)
{
    (void)service;
    VarMainWindow *self = VAR_MAIN_WINDOW (user_data);
    var_progress_overlay_set_fraction (self->progress_overlay, fraction);
    var_progress_overlay_set_detail (self->progress_overlay, file);
}

static void
on_create_finished (GObject *source_object, GAsyncResult *res, gpointer user_data)
{
    VarArchiveService *service = VAR_ARCHIVE_SERVICE (source_object);
    CreateContext *ctx = user_data;
    VarMainWindow *self = ctx->window;
    
    g_signal_handlers_disconnect_by_func (service, on_create_progress, self);
    
    GError *error = NULL;
    gboolean ok = var_archive_service_create_finish (service, res, &error);
    
    var_progress_overlay_hide (self->progress_overlay);
    
    if (!ok) {
        GtkWidget *dialog = gtk_message_dialog_new (GTK_WINDOW (self),
            GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
            "Creation failed: %s", error ? error->message : "Unknown error");
        gtk_window_present (GTK_WINDOW (dialog));
        g_signal_connect (dialog, "response", G_CALLBACK(gtk_window_destroy), NULL);
        if (error) g_error_free (error);
        update_status (self, "Creation failed");
    } else {
        update_status (self, "Archive created successfully");
        g_autoptr(GFile) new_file = g_file_new_for_path (ctx->dest_path);
        var_main_window_open_file (self, new_file);
    }
    
    create_context_free (ctx);
}

static void
on_files_to_compress_response (GtkDialog *dialog, int response, gpointer user_data)
{
    CreateContext *ctx = user_data;
    VarMainWindow *self = ctx->window;

    if (response != GTK_RESPONSE_ACCEPT) {
        gtk_window_destroy (GTK_WINDOW (dialog));
        create_context_free (ctx);
        return;
    }

    GtkFileChooser *chooser = GTK_FILE_CHOOSER (dialog);
    g_autoptr(GListModel) files_model = gtk_file_chooser_get_files (chooser);
    
    GPtrArray *files_array = g_ptr_array_new_full (g_list_model_get_n_items (files_model), g_free);
    for (guint i = 0; i < g_list_model_get_n_items (files_model); i++) {
        g_autoptr(GFile) file = g_list_model_get_item (files_model, i);
        g_ptr_array_add (files_array, g_file_get_path (file));
    }
    
    gtk_window_destroy (GTK_WINDOW (dialog));
    
    if (files_array->len == 0) {
        create_context_free (ctx);
        g_ptr_array_unref (files_array);
        return;
    }
    
    var_progress_overlay_show (self->progress_overlay, "Creating Archive...");
    
    VarArchiveService *service = var_archive_service_get_default ();
    g_signal_connect (service, "progress-changed", G_CALLBACK (on_create_progress), self);
    
    var_archive_service_create_async (service,
        ctx->dest_path,
        ctx->format,
        files_array,
        NULL, /* base_dir */
        ctx->password,
        ctx->level,
        NULL,
        on_create_finished,
        ctx);
        
    g_ptr_array_unref (files_array);
}

static void
on_create_settings_confirmed (const char *dest_path, VarFormatType format, VarCompressionLevel level, const char *password, gpointer user_data)
{
    VarMainWindow *self = VAR_MAIN_WINDOW (user_data);
    
    CreateContext *ctx = g_new0(CreateContext, 1);
    ctx->window = self;
    ctx->dest_path = g_strdup(dest_path);
    ctx->format = format;
    ctx->level = level;
    ctx->password = g_strdup(password);

    GtkWidget *dialog = gtk_file_chooser_dialog_new ("Select Files to Compress",
                                                     GTK_WINDOW (self),
                                                     GTK_FILE_CHOOSER_ACTION_OPEN,
                                                     "Cancel", GTK_RESPONSE_CANCEL,
                                                     "Compress", GTK_RESPONSE_ACCEPT,
                                                     NULL);
    gtk_file_chooser_set_select_multiple (GTK_FILE_CHOOSER (dialog), TRUE);
    g_signal_connect (dialog, "response", G_CALLBACK (on_files_to_compress_response), ctx);
    gtk_window_present (GTK_WINDOW (dialog));
}

static void
action_create (VarToolbar *toolbar, gpointer user_data)
{
    (void)toolbar;
    VarMainWindow *self = VAR_MAIN_WINDOW (user_data);
    
    VarCreateDialog *dialog = var_create_dialog_new (GTK_WINDOW (self), "new_archive");
    var_create_dialog_set_callback (dialog, on_create_settings_confirmed, self);
    gtk_window_present (GTK_WINDOW (dialog));
}

static void
action_properties (VarToolbar *toolbar, gpointer user_data)
{
    (void)toolbar;
    VarMainWindow *self = VAR_MAIN_WINDOW (user_data);
    if (!self->current_archive_path) return;
    
    /* TODO: fetch actual stats from service/model */
    guint64 size = 10485760; /* 10MB dummy */
    guint64 comp_size = 5242880; /* 5MB dummy */
    VarFormatType fmt = var_utils_detect_format (self->current_archive_path);
    
    VarPropertiesDialog *dialog = var_properties_dialog_new (GTK_WINDOW (self), self->current_archive_path, var_utils_format_name(fmt), size, comp_size, 42, 5);
    gtk_window_present (GTK_WINDOW (dialog));
}

/* ═══════════════════════════════════════════════════
   Initialization
   ═══════════════════════════════════════════════════ */

static void
var_main_window_finalize (GObject *object)
{
    VarMainWindow *self = VAR_MAIN_WINDOW (object);
    g_free (self->current_archive_path);
    G_OBJECT_CLASS (var_main_window_parent_class)->finalize (object);
}

static void
var_main_window_class_init (VarMainWindowClass *klass)
{
    GObjectClass *obj_class = G_OBJECT_CLASS (klass);
    obj_class->finalize = var_main_window_finalize;
}

static void
on_open_selected (GSimpleAction *action, GVariant *param, gpointer user_data)
{
    (void)action; (void)param;
    VarMainWindow *self = VAR_MAIN_WINDOW (user_data);
    g_autoptr(GPtrArray) paths = var_archive_view_get_selected_paths (self->archive_view);
    if (!paths || paths->len == 0) return;
    
    /* TODO: Open selected file using service */
    g_autofree char *msg = g_strdup_printf ("Opening %s...", (char*)g_ptr_array_index(paths, 0));
    update_status (self, msg);
}

static void
on_extract_selected (GSimpleAction *action, GVariant *param, gpointer user_data)
{
    (void)action; (void)param;
    VarMainWindow *self = VAR_MAIN_WINDOW (user_data);
    action_extract (self->toolbar, self); /* Just open the extract dialog for now */
}

static void
on_delete_selected (GSimpleAction *action, GVariant *param, gpointer user_data)
{
    (void)action; (void)param;
    VarMainWindow *self = VAR_MAIN_WINDOW (user_data);
    g_autoptr(GPtrArray) paths = var_archive_view_get_selected_paths (self->archive_view);
    if (!paths || paths->len == 0) return;
    
    /* TODO: Delete using service */
    g_autofree char *msg = g_strdup_printf ("Deleting %u items...", paths->len);
    update_status (self, msg);
}

static void
on_properties_selected (GSimpleAction *action, GVariant *param, gpointer user_data)
{
    (void)action; (void)param;
    VarMainWindow *self = VAR_MAIN_WINDOW (user_data);
    action_properties (self->toolbar, self);
}

static const GActionEntry win_actions[] = {
    { "open-selected",       on_open_selected,       NULL, NULL, NULL },
    { "extract-selected",    on_extract_selected,    NULL, NULL, NULL },
    { "delete-selected",     on_delete_selected,     NULL, NULL, NULL },
    { "properties-selected", on_properties_selected, NULL, NULL, NULL },
};

static void
var_main_window_init (VarMainWindow *self)
{
    gtk_window_set_default_size (GTK_WINDOW (self), 900, 650);
    gtk_widget_add_css_class (GTK_WIDGET (self), "var-window");

    g_action_map_add_action_entries (G_ACTION_MAP (self), win_actions, G_N_ELEMENTS (win_actions), self);

    /* Create UI components */
    self->header_bar = var_headerbar_new (GTK_WINDOW (self));
    self->toolbar = var_toolbar_new ();
    self->location_bar = var_location_bar_new ();
    self->archive_view = var_archive_view_new ();
    self->progress_overlay = var_progress_overlay_new ();

    /* Hook up toolbar signals */
    g_signal_connect (self->toolbar, "action-open", G_CALLBACK (action_open), self);
    g_signal_connect (self->toolbar, "action-extract", G_CALLBACK (action_extract), self);
    g_signal_connect (self->toolbar, "action-create", G_CALLBACK (action_create), self);
    g_signal_connect (self->toolbar, "action-properties", G_CALLBACK (action_properties), self);

    /* Main layout box */
    GtkWidget *main_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);

    /* Construct Header */
    GtkWidget *header_area = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_add_css_class (header_area, "var-header-area");
    gtk_box_append (GTK_BOX (header_area), GTK_WIDGET (self->header_bar));
    gtk_box_append (GTK_BOX (header_area), GTK_WIDGET (self->toolbar));
    gtk_box_append (GTK_BOX (main_box), header_area);

    /* Location Bar */
    GtkWidget *loc_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_margin_start (loc_box, 8);
    gtk_widget_set_margin_end (loc_box, 8);
    gtk_widget_set_margin_top (loc_box, 4);
    gtk_widget_set_margin_bottom (loc_box, 4);
    gtk_box_append (GTK_BOX (loc_box), GTK_WIDGET (self->location_bar));
    gtk_box_append (GTK_BOX (main_box), loc_box);

    /* Status Bar (Bottom) */
    GtkWidget *status_bar = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_widget_add_css_class (status_bar, "var-statusbar");
    gtk_widget_set_margin_start (status_bar, 12);
    gtk_widget_set_margin_end (status_bar, 12);
    gtk_widget_set_margin_top (status_bar, 4);
    gtk_widget_set_margin_bottom (status_bar, 4);
    
    self->status_label = gtk_label_new ("Ready");
    gtk_widget_set_halign (self->status_label, GTK_ALIGN_START);
    gtk_box_append (GTK_BOX (status_bar), self->status_label);

    /* Overlay for content + progress */
    self->overlay = gtk_overlay_new ();
    gtk_widget_set_vexpand (self->overlay, TRUE);
    gtk_widget_set_hexpand (self->overlay, TRUE);
    
    gtk_overlay_set_child (GTK_OVERLAY (self->overlay), GTK_WIDGET (self->archive_view));
    gtk_overlay_add_overlay (GTK_OVERLAY (self->overlay), GTK_WIDGET (self->progress_overlay));
    
    gtk_box_append (GTK_BOX (main_box), self->overlay);
    gtk_box_append (GTK_BOX (main_box), status_bar);

    /* Set window content */
    adw_application_window_set_content (ADW_APPLICATION_WINDOW (self), main_box);

    update_window_title (self);
}

VarMainWindow *
var_main_window_new (VarApplication *app)
{
    return g_object_new (VAR_TYPE_MAIN_WINDOW, "application", app, NULL);
}

void
var_main_window_open_file (VarMainWindow *self, GFile *file)
{
    g_return_if_fail (VAR_IS_MAIN_WINDOW (self));
    if (!file) return;

    g_free (self->current_archive_path);
    self->current_archive_path = g_file_get_path (file);
    self->is_modified = FALSE;

    var_progress_overlay_show (self->progress_overlay, "Opening Archive...");
    
    VarArchiveService *service = var_archive_service_get_default ();
    var_archive_service_open_async (service, self->current_archive_path, NULL,
                                     NULL, on_archive_opened, self);
}
