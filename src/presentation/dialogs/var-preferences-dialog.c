/*
 * VArchive - Professional Archive Manager for VAXP-OS
 * Copyright (C) 2026 VAXP Organization
 *
 * var-preferences-dialog.c — User preferences
 */

#include "var-preferences-dialog.h"

struct _VarPreferencesDialog {
    AdwPreferencesWindow parent_instance;
};

G_DEFINE_TYPE (VarPreferencesDialog, var_preferences_dialog, ADW_TYPE_PREFERENCES_WINDOW)

static void
var_preferences_dialog_init (VarPreferencesDialog *self)
{
    gtk_window_set_default_size (GTK_WINDOW (self), 600, 500);
    gtk_widget_add_css_class (GTK_WIDGET (self), "var-window");

    /* General Page */
    GtkWidget *gen_page = adw_preferences_page_new ();
    adw_preferences_page_set_title (ADW_PREFERENCES_PAGE (gen_page), "General");
    adw_preferences_page_set_icon_name (ADW_PREFERENCES_PAGE (gen_page), "preferences-system-symbolic");

    GtkWidget *ext_group = adw_preferences_group_new ();
    adw_preferences_group_set_title (ADW_PREFERENCES_GROUP (ext_group), "Extraction");

    GtkWidget *ext_row = adw_action_row_new ();
    adw_preferences_row_set_title (ADW_PREFERENCES_ROW (ext_row), "Default Destination");
    adw_action_row_set_subtitle (ADW_ACTION_ROW (ext_row), "Where files are extracted to by default.");

    GtkStringList *ext_model = gtk_string_list_new (NULL);
    gtk_string_list_append (ext_model, "Ask Every Time");
    gtk_string_list_append (ext_model, "Current Folder");
    gtk_string_list_append (ext_model, "Downloads Folder");
    GtkWidget *ext_combo = adw_combo_row_new ();
    adw_combo_row_set_model (ADW_COMBO_ROW (ext_combo), G_LIST_MODEL (ext_model));
    adw_preferences_group_add (ADW_PREFERENCES_GROUP (ext_group), ext_combo);
    
    adw_preferences_page_add (ADW_PREFERENCES_PAGE (gen_page), ADW_PREFERENCES_GROUP (ext_group));

    /* View Group */
    GtkWidget *view_group = adw_preferences_group_new ();
    adw_preferences_group_set_title (ADW_PREFERENCES_GROUP (view_group), "View");

    GtkWidget *hidden_row = adw_action_row_new ();
    adw_preferences_row_set_title (ADW_PREFERENCES_ROW (hidden_row), "Show Hidden Files");
    GtkWidget *hidden_switch = gtk_switch_new ();
    gtk_widget_set_valign (hidden_switch, GTK_ALIGN_CENTER);
    adw_action_row_add_suffix (ADW_ACTION_ROW (hidden_row), hidden_switch);
    adw_preferences_group_add (ADW_PREFERENCES_GROUP (view_group), hidden_row);

    adw_preferences_page_add (ADW_PREFERENCES_PAGE (gen_page), ADW_PREFERENCES_GROUP (view_group));

    /* Add pages */
    adw_preferences_window_add (ADW_PREFERENCES_WINDOW (self), ADW_PREFERENCES_PAGE (gen_page));
}

static void
var_preferences_dialog_class_init (VarPreferencesDialogClass *klass)
{
    (void)klass;
}

VarPreferencesDialog *
var_preferences_dialog_new (GtkWindow *parent)
{
    return g_object_new (VAR_TYPE_PREFERENCES_DIALOG, "transient-for", parent, NULL);
}
