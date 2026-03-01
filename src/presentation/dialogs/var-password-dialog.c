/*
 * VArchive - Professional Archive Manager for VAXP-OS
 * Copyright (C) 2026 VAXP Organization
 *
 * var-password-dialog.c — Password prompt dialog
 */

#include "var-password-dialog.h"

struct _VarPasswordDialog {
    AdwWindow parent_instance;

    GtkWidget *password_entry;
    GtkWidget *ok_button;

    VarPasswordCallback callback;
    gpointer callback_data;
};

G_DEFINE_TYPE (VarPasswordDialog, var_password_dialog, ADW_TYPE_WINDOW)

static void
on_ok_clicked (GtkButton *btn, gpointer user_data)
{
    VarPasswordDialog *self = VAR_PASSWORD_DIALOG (user_data);
    (void)btn;
    
    if (self->callback) {
        const char *password = gtk_editable_get_text (GTK_EDITABLE (self->password_entry));
        self->callback (password, self->callback_data);
    }
    
    gtk_window_close (GTK_WINDOW (self));
}

static void
on_password_changed (GtkEditable *editable, gpointer user_data)
{
    VarPasswordDialog *self = VAR_PASSWORD_DIALOG (user_data);
    const char *text = gtk_editable_get_text (editable);
    gtk_widget_set_sensitive (self->ok_button, text && *text);
}

static void
var_password_dialog_init (VarPasswordDialog *self)
{
    gtk_window_set_default_size (GTK_WINDOW (self), 350, 200);
    gtk_window_set_resizable (GTK_WINDOW (self), FALSE);
    gtk_window_set_modal (GTK_WINDOW (self), TRUE);
    gtk_widget_add_css_class (GTK_WIDGET (self), "var-window");

    GtkWidget *box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
    
    /* Header */
    GtkWidget *header = adw_header_bar_new ();
    adw_header_bar_set_title_widget (ADW_HEADER_BAR (header), adw_window_title_new ("Password Required", NULL));
    
    self->ok_button = gtk_button_new_with_label ("Unlock");
    gtk_widget_add_css_class (self->ok_button, "suggested-action");
    gtk_widget_set_sensitive (self->ok_button, FALSE);
    g_signal_connect (self->ok_button, "clicked", G_CALLBACK (on_ok_clicked), self);
    adw_header_bar_pack_end (ADW_HEADER_BAR (header), self->ok_button);
    
    gtk_box_append (GTK_BOX (box), header);

    /* Content */
    GtkWidget *status_page = adw_status_page_new ();
    adw_status_page_set_icon_name (ADW_STATUS_PAGE (status_page), "dialog-password-symbolic");
    adw_status_page_set_title (ADW_STATUS_PAGE (status_page), "Encrypted Archive");
    adw_status_page_set_description (ADW_STATUS_PAGE (status_page), "Please enter the password to access this archive.");
    
    self->password_entry = gtk_password_entry_new ();
    gtk_password_entry_set_show_peek_icon (GTK_PASSWORD_ENTRY (self->password_entry), TRUE);
    gtk_widget_set_margin_start (self->password_entry, 32);
    gtk_widget_set_margin_end (self->password_entry, 32);
    gtk_widget_set_margin_bottom (self->password_entry, 16);
    
    g_signal_connect (self->password_entry, "changed", G_CALLBACK (on_password_changed), self);
    g_signal_connect_swapped (self->password_entry, "activate", G_CALLBACK (on_ok_clicked), self);
    
    /* Assemble */
    GtkWidget *vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 16);
    gtk_box_append (GTK_BOX (vbox), status_page);
    gtk_box_append (GTK_BOX (vbox), self->password_entry);

    gtk_box_append (GTK_BOX (box), vbox);
    adw_window_set_content (ADW_WINDOW (self), box);
}

static void
var_password_dialog_finalize (GObject *object)
{
    G_OBJECT_CLASS (var_password_dialog_parent_class)->finalize (object);
}

static void
var_password_dialog_class_init (VarPasswordDialogClass *klass)
{
    GObjectClass *obj_class = G_OBJECT_CLASS (klass);
    obj_class->finalize = var_password_dialog_finalize;
}

VarPasswordDialog *
var_password_dialog_new (GtkWindow *parent, const char *archive_name)
{
    VarPasswordDialog *self = g_object_new (VAR_TYPE_PASSWORD_DIALOG, "transient-for", parent, NULL);
    if (archive_name) {
        g_autofree char *desc = g_strdup_printf ("Please enter the password to access '%s'.", archive_name);
        GtkWidget *content = adw_window_get_content (ADW_WINDOW (self));
        GtkWidget *vbox = gtk_widget_get_last_child (content);
        GtkWidget *status_page = gtk_widget_get_first_child (vbox);
        adw_status_page_set_description (ADW_STATUS_PAGE (status_page), desc);
    }
    return self;
}

void
var_password_dialog_set_callback (VarPasswordDialog *self, VarPasswordCallback callback, gpointer user_data)
{
    self->callback = callback;
    self->callback_data = user_data;
}
