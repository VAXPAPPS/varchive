/*
 * VArchive - Professional Archive Manager for VAXP-OS
 * Copyright (C) 2026 VAXP Organization
 *
 * var-properties-dialog.c — Archive properties panel
 */

#include "var-properties-dialog.h"
#include "../../core/var-utils.h"

struct _VarPropertiesDialog {
    AdwWindow parent_instance;
};

G_DEFINE_TYPE (VarPropertiesDialog, var_properties_dialog, ADW_TYPE_WINDOW)

static GtkWidget*
create_info_row (const char *title, const char *value)
{
    GtkWidget *row = adw_action_row_new ();
    adw_preferences_row_set_title (ADW_PREFERENCES_ROW (row), title);
    
    GtkWidget *label = gtk_label_new (value);
    gtk_label_set_selectable (GTK_LABEL (label), TRUE);
    gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
    gtk_widget_add_css_class (label, "dim-label");
    adw_action_row_add_suffix (ADW_ACTION_ROW (row), label);
    
    return row;
}

static void
var_properties_dialog_init (VarPropertiesDialog *self)
{
    gtk_window_set_default_size (GTK_WINDOW (self), 400, 450);
    gtk_window_set_resizable (GTK_WINDOW (self), FALSE);
    gtk_window_set_modal (GTK_WINDOW (self), TRUE);
    gtk_widget_add_css_class (GTK_WIDGET (self), "var-window");
}

static void
var_properties_dialog_class_init (VarPropertiesDialogClass *klass)
{
    (void)klass;
}

VarPropertiesDialog *
var_properties_dialog_new (GtkWindow *parent,
                           const char *archive_path,
                           const char *format_name,
                           guint64     total_size,
                           guint64     compressed_size,
                           guint       file_count,
                           guint       dir_count)
{
    VarPropertiesDialog *self = g_object_new (VAR_TYPE_PROPERTIES_DIALOG, "transient-for", parent, NULL);

    GtkWidget *box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
    
    /* Header */
    GtkWidget *header = adw_header_bar_new ();
    adw_header_bar_set_title_widget (ADW_HEADER_BAR (header), adw_window_title_new ("Properties", NULL));
    gtk_box_append (GTK_BOX (box), header);

    /* Content */
    GtkWidget *pref_page = adw_preferences_page_new ();
    
    GtkWidget *group = adw_preferences_group_new ();
    adw_preferences_page_add (ADW_PREFERENCES_PAGE (pref_page), ADW_PREFERENCES_GROUP (group));
    
    /* Icon and Name */
    g_autofree char *basename = g_path_get_basename (archive_path);
    GtkWidget *avatar = adw_avatar_new (64, basename, TRUE);
    GtkWidget *name_label = gtk_label_new (basename);
    gtk_label_set_wrap (GTK_LABEL (name_label), TRUE);
    gtk_widget_add_css_class (name_label, "title-2");
    
    GtkWidget *top_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
    gtk_widget_set_margin_top (top_box, 24);
    gtk_widget_set_margin_bottom (top_box, 24);
    gtk_widget_set_halign (top_box, GTK_ALIGN_CENTER);
    gtk_box_append (GTK_BOX (top_box), avatar);
    gtk_box_append (GTK_BOX (top_box), name_label);
    
    gtk_box_append (GTK_BOX (box), top_box);
    
    /* Details */
    g_autofree char *s_uncomp = var_utils_format_size (total_size);
    g_autofree char *s_comp = var_utils_format_size (compressed_size);
    /* Calculate ratio cleanly */
    double ratio = 0;
    if (total_size > 0 && compressed_size > 0) {
       ratio = ((double)total_size / (double)compressed_size);
    }
    g_autofree char *s_ratio = g_strdup_printf ("%.1fx", ratio);
    
    g_autofree char *s_count = g_strdup_printf ("%u files, %u folders", file_count, dir_count);

    adw_preferences_group_add (ADW_PREFERENCES_GROUP (group), create_info_row ("Location", archive_path));
    adw_preferences_group_add (ADW_PREFERENCES_GROUP (group), create_info_row ("Format", format_name));
    adw_preferences_group_add (ADW_PREFERENCES_GROUP (group), create_info_row ("Uncompressed Size", s_uncomp));
    adw_preferences_group_add (ADW_PREFERENCES_GROUP (group), create_info_row ("Compressed Size", s_comp));
    if (ratio > 0) {
        adw_preferences_group_add (ADW_PREFERENCES_GROUP (group), create_info_row ("Ratio", s_ratio));
    }
    adw_preferences_group_add (ADW_PREFERENCES_GROUP (group), create_info_row ("Contents", s_count));

    gtk_box_append (GTK_BOX (box), pref_page);
    adw_window_set_content (ADW_WINDOW (self), box);
    
    return self;
}
