/*
 * VArchive - Professional Archive Manager for VAXP-OS
 * Copyright (C) 2026 VAXP Organization
 *
 * var-extract-dialog.c — Archive extraction dialog
 */

#include "var-extract-dialog.h"

struct _VarExtractDialog {
    AdwWindow parent_instance;

    GtkWidget *dest_button;
    GtkWidget *all_files_radio;
    GtkWidget *selected_files_radio;
    GtkWidget *create_folder_switch;
    GtkWidget *overwrite_combo;
    GtkWidget *extract_button;

    char *current_dest_path;

    VarExtractSettingsCallback callback;
    gpointer callback_data;
};

G_DEFINE_TYPE (VarExtractDialog, var_extract_dialog, ADW_TYPE_WINDOW)

static void
on_dest_chooser_response (GtkDialog *dlg, int response, gpointer data)
{
    VarExtractDialog *d = VAR_EXTRACT_DIALOG (data);
    if (response == GTK_RESPONSE_ACCEPT) {
        g_autoptr(GFile) file = gtk_file_chooser_get_file (GTK_FILE_CHOOSER (dlg));
        g_free (d->current_dest_path);
        d->current_dest_path = g_file_get_path (file);
        g_autofree char *basename = g_file_get_basename (file);
        gtk_button_set_label (GTK_BUTTON (d->dest_button), basename);
    }
    gtk_window_destroy (GTK_WINDOW (dlg));
}

static void
on_dest_button_clicked (GtkButton *btn, gpointer user_data)
{
    VarExtractDialog *self = VAR_EXTRACT_DIALOG (user_data);
    (void)btn;
    
    GtkWidget *dialog = gtk_file_chooser_dialog_new ("Select Destination Folder",
                                                     GTK_WINDOW (self),
                                                     GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
                                                     "Cancel", GTK_RESPONSE_CANCEL,
                                                     "Select", GTK_RESPONSE_ACCEPT,
                                                     NULL);
    
    if (self->current_dest_path) {
        g_autoptr(GFile) file = g_file_new_for_path (self->current_dest_path);
        gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dialog), file, NULL);
    }
    
    g_signal_connect (dialog, "response", G_CALLBACK (on_dest_chooser_response), self);
    
    gtk_window_present (GTK_WINDOW (dialog));
}

static void
on_extract_clicked (GtkButton *btn, gpointer user_data)
{
    VarExtractDialog *self = VAR_EXTRACT_DIALOG (user_data);
    (void)btn;
    
    if (self->callback && self->current_dest_path) {
        gboolean extract_all = gtk_check_button_get_active (GTK_CHECK_BUTTON (self->all_files_radio));
        gboolean create_folder = gtk_switch_get_active (GTK_SWITCH (self->create_folder_switch));
        
        const char *ow_id = adw_combo_row_get_selected_item (ADW_COMBO_ROW (self->overwrite_combo)) ? 
                             gtk_string_object_get_string (GTK_STRING_OBJECT (g_list_model_get_item (
                                adw_combo_row_get_model (ADW_COMBO_ROW (self->overwrite_combo)), 
                                adw_combo_row_get_selected (ADW_COMBO_ROW (self->overwrite_combo))))) : NULL;

        VarOverwritePolicy policy = VAR_OVERWRITE_ASK;
        if (ow_id) {
            if (g_strcmp0(ow_id, "Overwrite All") == 0) policy = VAR_OVERWRITE_ALWAYS;
            else if (g_strcmp0(ow_id, "Skip Existing") == 0) policy = VAR_OVERWRITE_SKIP;
        }

        self->callback (self->current_dest_path, extract_all, create_folder, policy, self->callback_data);
    }
    
    gtk_window_close (GTK_WINDOW (self));
}

static void
var_extract_dialog_init (VarExtractDialog *self)
{
    gtk_window_set_default_size (GTK_WINDOW (self), 450, 400);
    gtk_window_set_resizable (GTK_WINDOW (self), FALSE);
    gtk_window_set_modal (GTK_WINDOW (self), TRUE);
    gtk_widget_add_css_class (GTK_WIDGET (self), "var-window");

    self->current_dest_path = g_strdup (g_get_home_dir ());

    GtkWidget *box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
    
    /* Header */
    GtkWidget *header = adw_header_bar_new ();
    GtkWidget *title = adw_window_title_new ("Extract Archive", NULL);
    adw_header_bar_set_title_widget (ADW_HEADER_BAR (header), title);
    
    self->extract_button = gtk_button_new_with_label ("Extract");
    gtk_widget_add_css_class (self->extract_button, "suggested-action");
    g_signal_connect (self->extract_button, "clicked", G_CALLBACK (on_extract_clicked), self);
    adw_header_bar_pack_end (ADW_HEADER_BAR (header), self->extract_button);
    
    gtk_box_append (GTK_BOX (box), header);

    /* Content Preferences */
    GtkWidget *pref_page = adw_preferences_page_new ();
    
    /* Destination Group */
    GtkWidget *dest_group = adw_preferences_group_new ();
    adw_preferences_group_set_title (ADW_PREFERENCES_GROUP (dest_group), "Destination");
    
    GtkWidget *dest_row = adw_action_row_new ();
    adw_preferences_row_set_title (ADW_PREFERENCES_ROW (dest_row), "Folder");
    
    self->dest_button = gtk_button_new_with_label ("Home");
    gtk_widget_set_valign (self->dest_button, GTK_ALIGN_CENTER);
    gtk_widget_add_css_class (self->dest_button, "flat");
    g_signal_connect (self->dest_button, "clicked", G_CALLBACK (on_dest_button_clicked), self);
    adw_action_row_add_suffix (ADW_ACTION_ROW (dest_row), self->dest_button);
    
    adw_preferences_group_add (ADW_PREFERENCES_GROUP (dest_group), dest_row);
    
    /* Options Group */
    GtkWidget *opt_group = adw_preferences_group_new ();
    adw_preferences_group_set_title (ADW_PREFERENCES_GROUP (opt_group), "Options");
    
    GtkWidget *create_row = adw_action_row_new ();
    adw_preferences_row_set_title (ADW_PREFERENCES_ROW (create_row), "Create folder using archive name");
    self->create_folder_switch = gtk_switch_new ();
    gtk_widget_set_valign (self->create_folder_switch, GTK_ALIGN_CENTER);
    gtk_switch_set_active (GTK_SWITCH (self->create_folder_switch), TRUE);
    adw_action_row_add_suffix (ADW_ACTION_ROW (create_row), self->create_folder_switch);
    adw_preferences_group_add (ADW_PREFERENCES_GROUP (opt_group), create_row);
    
    /* Overwrite combo */
    GtkStringList *ow_model = gtk_string_list_new (NULL);
    gtk_string_list_append (ow_model, "Ask");
    gtk_string_list_append (ow_model, "Overwrite All");
    gtk_string_list_append (ow_model, "Skip Existing");
    
    self->overwrite_combo = adw_combo_row_new ();
    adw_preferences_row_set_title (ADW_PREFERENCES_ROW (self->overwrite_combo), "If files exist");
    adw_combo_row_set_model (ADW_COMBO_ROW (self->overwrite_combo), G_LIST_MODEL (ow_model));
    adw_preferences_group_add (ADW_PREFERENCES_GROUP (opt_group), self->overwrite_combo);
    
    /* Files Group */
    GtkWidget *file_group = adw_preferences_group_new ();
    adw_preferences_group_set_title (ADW_PREFERENCES_GROUP (file_group), "Files");
    
    GtkWidget *all_row = adw_action_row_new ();
    adw_preferences_row_set_title (ADW_PREFERENCES_ROW (all_row), "All files");
    self->all_files_radio = gtk_check_button_new ();
    gtk_check_button_set_active (GTK_CHECK_BUTTON (self->all_files_radio), TRUE);
    adw_action_row_add_prefix (ADW_ACTION_ROW (all_row), self->all_files_radio);
    adw_preferences_group_add (ADW_PREFERENCES_GROUP (file_group), all_row);
    
    GtkWidget *sel_row = adw_action_row_new ();
    adw_preferences_row_set_title (ADW_PREFERENCES_ROW (sel_row), "Selected files only");
    self->selected_files_radio = gtk_check_button_new ();
    gtk_check_button_set_group (GTK_CHECK_BUTTON (self->selected_files_radio), GTK_CHECK_BUTTON (self->all_files_radio));
    adw_action_row_add_prefix (ADW_ACTION_ROW (sel_row), self->selected_files_radio);
    adw_preferences_group_add (ADW_PREFERENCES_GROUP (file_group), sel_row);


    adw_preferences_page_add (ADW_PREFERENCES_PAGE (pref_page), ADW_PREFERENCES_GROUP (dest_group));
    adw_preferences_page_add (ADW_PREFERENCES_PAGE (pref_page), ADW_PREFERENCES_GROUP (file_group));
    adw_preferences_page_add (ADW_PREFERENCES_PAGE (pref_page), ADW_PREFERENCES_GROUP (opt_group));

    gtk_box_append (GTK_BOX (box), pref_page);
    adw_window_set_content (ADW_WINDOW (self), box);
}

static void
var_extract_dialog_finalize (GObject *object)
{
    VarExtractDialog *self = VAR_EXTRACT_DIALOG (object);
    g_free (self->current_dest_path);
    G_OBJECT_CLASS (var_extract_dialog_parent_class)->finalize (object);
}

static void
var_extract_dialog_class_init (VarExtractDialogClass *klass)
{
    GObjectClass *obj_class = G_OBJECT_CLASS (klass);
    obj_class->finalize = var_extract_dialog_finalize;
}

VarExtractDialog *
var_extract_dialog_new (GtkWindow *parent, const char *archive_path, gboolean has_selection)
{
    VarExtractDialog *self = g_object_new (VAR_TYPE_EXTRACT_DIALOG, "transient-for", parent, NULL);
    
    if (archive_path) {
        g_autofree char *basename = g_path_get_basename (archive_path);
        g_autofree char *title = g_strdup_printf ("Extract '%s'", basename);
        adw_window_title_set_subtitle (ADW_WINDOW_TITLE (adw_header_bar_get_title_widget (ADW_HEADER_BAR (gtk_widget_get_first_child (adw_window_get_content (ADW_WINDOW (self)))))), title);
    }
    
    if (!has_selection) {
        gtk_widget_set_sensitive (self->selected_files_radio, FALSE);
    }
    
    return self;
}

void
var_extract_dialog_set_callback (VarExtractDialog *self, VarExtractSettingsCallback callback, gpointer user_data)
{
    self->callback = callback;
    self->callback_data = user_data;
}
