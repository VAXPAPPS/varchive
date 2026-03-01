/*
 * VArchive - Professional Archive Manager for VAXP-OS
 * Copyright (C) 2026 VAXP Organization
 *
 * var-create-dialog.c — Archive creation dialog
 */

#include "var-create-dialog.h"

struct _VarCreateDialog {
    AdwWindow parent_instance;

    GtkWidget *name_entry;
    GtkWidget *format_combo;
    GtkWidget *level_combo;
    GtkWidget *password_entry;
    GtkWidget *show_password_btn;
    GtkWidget *dest_button;
    GtkWidget *create_button;

    char *current_dest_dir;

    VarCreateSettingsCallback callback;
    gpointer callback_data;
};

G_DEFINE_TYPE (VarCreateDialog, var_create_dialog, ADW_TYPE_WINDOW)

static void
on_dest_chooser_response (GtkDialog *dlg, int response, gpointer data)
{
    VarCreateDialog *d = VAR_CREATE_DIALOG (data);
    if (response == GTK_RESPONSE_ACCEPT) {
        g_autoptr(GFile) file = gtk_file_chooser_get_file (GTK_FILE_CHOOSER (dlg));
        g_free (d->current_dest_dir);
        d->current_dest_dir = g_file_get_path (file);
        g_autofree char *basename = g_file_get_basename (file);
        gtk_button_set_label (GTK_BUTTON (d->dest_button), basename);
    }
    gtk_window_destroy (GTK_WINDOW (dlg));
}

static void
on_dest_button_clicked (GtkButton *btn, gpointer user_data)
{
    VarCreateDialog *self = VAR_CREATE_DIALOG (user_data);
    (void)btn;
    
    GtkWidget *dialog = gtk_file_chooser_dialog_new ("Select Destination Folder",
                                                     GTK_WINDOW (self),
                                                     GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
                                                     "Cancel", GTK_RESPONSE_CANCEL,
                                                     "Select", GTK_RESPONSE_ACCEPT,
                                                     NULL);
    
    if (self->current_dest_dir) {
        g_autoptr(GFile) file = g_file_new_for_path (self->current_dest_dir);
        gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dialog), file, NULL);
    }
    
    g_signal_connect (dialog, "response", G_CALLBACK (on_dest_chooser_response), self);
    
    gtk_window_present (GTK_WINDOW (dialog));
}

static void
on_create_clicked (GtkButton *btn, gpointer user_data)
{
    VarCreateDialog *self = VAR_CREATE_DIALOG (user_data);
    (void)btn;
    
    if (self->callback && self->current_dest_dir) {
        const char *filename = gtk_editable_get_text (GTK_EDITABLE (self->name_entry));
        if (!filename || !*filename) filename = "archive";
        
        g_autofree char *full_path = g_build_filename (self->current_dest_dir, filename, NULL);
        
        /* Map format combo index to VarFormatType */
        guint fmt_idx = adw_combo_row_get_selected (ADW_COMBO_ROW (self->format_combo));
        VarFormatType format = VAR_FORMAT_ZIP;
        if (fmt_idx == 1) format = VAR_FORMAT_TAR_GZ;
        else if (fmt_idx == 2) format = VAR_FORMAT_TAR_XZ;
        else if (fmt_idx == 3) format = VAR_FORMAT_SEVENZ;
        
        /* Map level combo index */
        guint lvl_idx = adw_combo_row_get_selected (ADW_COMBO_ROW (self->level_combo));
        VarCompressionLevel level = VAR_COMPRESS_NORMAL;
        if (lvl_idx == 0) level = VAR_COMPRESS_STORE;
        else if (lvl_idx == 1) level = VAR_COMPRESS_FAST;
        else if (lvl_idx == 3) level = VAR_COMPRESS_BEST;
        
        const char *password = gtk_editable_get_text (GTK_EDITABLE (self->password_entry));
        if (password && !*password) password = NULL;

        /* Append extension if not present */
        const char *ext = "";
        if (format == VAR_FORMAT_ZIP) ext = ".zip";
        else if (format == VAR_FORMAT_TAR_GZ) ext = ".tar.gz";
        else if (format == VAR_FORMAT_TAR_XZ) ext = ".tar.xz";
        else if (format == VAR_FORMAT_SEVENZ) ext = ".7z";
        
        g_autofree char *final_path = NULL;
        if (!g_str_has_suffix (full_path, ext)) {
            final_path = g_strdup_printf ("%s%s", full_path, ext);
        } else {
            final_path = g_strdup (full_path);
        }

        self->callback (final_path, format, level, password, self->callback_data);
    }
    
    gtk_window_close (GTK_WINDOW (self));
}

static void
var_create_dialog_init (VarCreateDialog *self)
{
    gtk_window_set_default_size (GTK_WINDOW (self), 450, 400);
    gtk_window_set_resizable (GTK_WINDOW (self), FALSE);
    gtk_window_set_modal (GTK_WINDOW (self), TRUE);
    gtk_widget_add_css_class (GTK_WIDGET (self), "var-window");

    self->current_dest_dir = g_strdup (g_get_home_dir ());

    GtkWidget *box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
    
    /* Header */
    GtkWidget *header = adw_header_bar_new ();
    adw_header_bar_set_title_widget (ADW_HEADER_BAR (header), adw_window_title_new ("Create Archive", NULL));
    
    self->create_button = gtk_button_new_with_label ("Create");
    gtk_widget_add_css_class (self->create_button, "suggested-action");
    g_signal_connect (self->create_button, "clicked", G_CALLBACK (on_create_clicked), self);
    adw_header_bar_pack_end (ADW_HEADER_BAR (header), self->create_button);
    
    gtk_box_append (GTK_BOX (box), header);

    /* Content */
    GtkWidget *pref_page = adw_preferences_page_new ();
    
    /* Main settings */
    GtkWidget *main_group = adw_preferences_group_new ();
    
    /* Filename */
    GtkWidget *name_row = adw_action_row_new ();
    adw_preferences_row_set_title (ADW_PREFERENCES_ROW (name_row), "Filename");
    self->name_entry = gtk_entry_new ();
    gtk_widget_set_valign (self->name_entry, GTK_ALIGN_CENTER);
    gtk_widget_set_hexpand (self->name_entry, TRUE);
    adw_action_row_add_suffix (ADW_ACTION_ROW (name_row), self->name_entry);
    adw_preferences_group_add (ADW_PREFERENCES_GROUP (main_group), name_row);
    
    /* Destination */
    GtkWidget *dest_row = adw_action_row_new ();
    adw_preferences_row_set_title (ADW_PREFERENCES_ROW (dest_row), "Location");
    self->dest_button = gtk_button_new_with_label ("Home");
    gtk_widget_set_valign (self->dest_button, GTK_ALIGN_CENTER);
    gtk_widget_add_css_class (self->dest_button, "flat");
    g_signal_connect (self->dest_button, "clicked", G_CALLBACK (on_dest_button_clicked), self);
    adw_action_row_add_suffix (ADW_ACTION_ROW (dest_row), self->dest_button);
    adw_preferences_group_add (ADW_PREFERENCES_GROUP (main_group), dest_row);
    
    /* Format */
    GtkStringList *fmt_model = gtk_string_list_new (NULL);
    gtk_string_list_append (fmt_model, "ZIP (Standard)");
    gtk_string_list_append (fmt_model, "TAR.GZ (Linux Native)");
    gtk_string_list_append (fmt_model, "TAR.XZ (High Compression)");
    gtk_string_list_append (fmt_model, "7Z (Best Compression)");
    self->format_combo = adw_combo_row_new ();
    adw_preferences_row_set_title (ADW_PREFERENCES_ROW (self->format_combo), "Format");
    adw_combo_row_set_model (ADW_COMBO_ROW (self->format_combo), G_LIST_MODEL (fmt_model));
    adw_preferences_group_add (ADW_PREFERENCES_GROUP (main_group), self->format_combo);

    /* Compression Level */
    GtkStringList *lvl_model = gtk_string_list_new (NULL);
    gtk_string_list_append (lvl_model, "Store (None)");
    gtk_string_list_append (lvl_model, "Fastest");
    gtk_string_list_append (lvl_model, "Normal");
    gtk_string_list_append (lvl_model, "Maximum");
    self->level_combo = adw_combo_row_new ();
    adw_preferences_row_set_title (ADW_PREFERENCES_ROW (self->level_combo), "Compression Level");
    adw_combo_row_set_model (ADW_COMBO_ROW (self->level_combo), G_LIST_MODEL (lvl_model));
    adw_combo_row_set_selected (ADW_COMBO_ROW (self->level_combo), 2); /* Default to normal */
    adw_preferences_group_add (ADW_PREFERENCES_GROUP (main_group), self->level_combo);
    
    /* Password */
    GtkWidget *pwd_group = adw_preferences_group_new ();
    adw_preferences_group_set_title (ADW_PREFERENCES_GROUP (pwd_group), "Security");
    
    GtkWidget *pwd_row = adw_action_row_new ();
    adw_preferences_row_set_title (ADW_PREFERENCES_ROW (pwd_row), "Password");
    self->password_entry = gtk_password_entry_new ();
    gtk_password_entry_set_show_peek_icon (GTK_PASSWORD_ENTRY (self->password_entry), TRUE);
    gtk_widget_set_valign (self->password_entry, GTK_ALIGN_CENTER);
    adw_action_row_add_suffix (ADW_ACTION_ROW (pwd_row), self->password_entry);
    adw_preferences_group_add (ADW_PREFERENCES_GROUP (pwd_group), pwd_row);
    
    adw_preferences_page_add (ADW_PREFERENCES_PAGE (pref_page), ADW_PREFERENCES_GROUP (main_group));
    adw_preferences_page_add (ADW_PREFERENCES_PAGE (pref_page), ADW_PREFERENCES_GROUP (pwd_group));

    gtk_box_append (GTK_BOX (box), pref_page);
    adw_window_set_content (ADW_WINDOW (self), box);
}

static void
var_create_dialog_finalize (GObject *object)
{
    VarCreateDialog *self = VAR_CREATE_DIALOG (object);
    g_free (self->current_dest_dir);
    G_OBJECT_CLASS (var_create_dialog_parent_class)->finalize (object);
}

static void
var_create_dialog_class_init (VarCreateDialogClass *klass)
{
    GObjectClass *obj_class = G_OBJECT_CLASS (klass);
    obj_class->finalize = var_create_dialog_finalize;
}

VarCreateDialog *
var_create_dialog_new (GtkWindow *parent, const char *suggested_name)
{
    VarCreateDialog *self = g_object_new (VAR_TYPE_CREATE_DIALOG, "transient-for", parent, NULL);
    if (suggested_name) {
        gtk_editable_set_text (GTK_EDITABLE (self->name_entry), suggested_name);
    }
    return self;
}

void
var_create_dialog_set_callback (VarCreateDialog *self, VarCreateSettingsCallback callback, gpointer user_data)
{
    self->callback = callback;
    self->callback_data = user_data;
}
