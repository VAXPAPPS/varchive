/*
 * VArchive - Professional Archive Manager for VAXP-OS
 * Copyright (C) 2026 VAXP Organization
 *
 * var-progress-overlay.c — Progress overlay for async operations
 */

#include "var-progress-overlay.h"

struct _VarProgressOverlay {
    GtkBox       parent_instance;

    GtkWidget   *title_label;
    GtkWidget   *detail_label;
    GtkWidget   *progress_bar;
    GtkWidget   *cancel_btn;

    GCallback    cancel_cb;
    gpointer     cancel_data;
};

G_DEFINE_TYPE (VarProgressOverlay, var_progress_overlay, GTK_TYPE_BOX)

static void
on_cancel_clicked (GtkButton *btn, gpointer user_data)
{
    (void)btn;
    VarProgressOverlay *self = VAR_PROGRESS_OVERLAY (user_data);
    if (self->cancel_cb) {
        void (*callback)(gpointer) = (void(*)(gpointer))self->cancel_cb;
        callback (self->cancel_data);
    }
    gtk_widget_set_sensitive (self->cancel_btn, FALSE);
    gtk_label_set_text (GTK_LABEL (self->detail_label), "Cancelling...");
}

static void
var_progress_overlay_class_init (VarProgressOverlayClass *klass)
{
    (void)klass;
}

static void
var_progress_overlay_init (VarProgressOverlay *self)
{
    gtk_orientable_set_orientation (GTK_ORIENTABLE (self), GTK_ORIENTATION_VERTICAL);
    gtk_widget_set_valign (GTK_WIDGET (self), GTK_ALIGN_CENTER);
    gtk_widget_set_halign (GTK_WIDGET (self), GTK_ALIGN_CENTER);
    gtk_widget_add_css_class (GTK_WIDGET (self), "var-progress-overlay");

    self->cancel_cb = NULL;
    self->cancel_data = NULL;

    /* Internal layout box with glassmorphism styling */
    GtkWidget *box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
    gtk_widget_add_css_class (box, "var-progress-box");
    gtk_box_append (GTK_BOX (self), box);

    /* Title */
    self->title_label = gtk_label_new ("Working...");
    gtk_widget_add_css_class (self->title_label, "title");
    gtk_box_append (GTK_BOX (box), self->title_label);

    /* Progress bar */
    self->progress_bar = gtk_progress_bar_new ();
    gtk_widget_add_css_class (self->progress_bar, "var-progress");
    gtk_progress_bar_set_show_text (GTK_PROGRESS_BAR (self->progress_bar), TRUE);
    gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (self->progress_bar), 0.0);
    gtk_box_append (GTK_BOX (box), self->progress_bar);

    /* Detail / Current file */
    self->detail_label = gtk_label_new ("");
    gtk_widget_add_css_class (self->detail_label, "detail");
    gtk_label_set_ellipsize (GTK_LABEL (self->detail_label), PANGO_ELLIPSIZE_MIDDLE);
    gtk_widget_set_size_request (self->detail_label, 300, -1);
    gtk_box_append (GTK_BOX (box), self->detail_label);

    /* Cancel button */
    self->cancel_btn = gtk_button_new_with_label ("Cancel");
    gtk_widget_set_halign (self->cancel_btn, GTK_ALIGN_CENTER);
    gtk_widget_add_css_class (self->cancel_btn, "destructive-action");
    gtk_widget_set_margin_top (self->cancel_btn, 8);
    g_signal_connect (self->cancel_btn, "clicked", G_CALLBACK (on_cancel_clicked), self);
    gtk_box_append (GTK_BOX (box), self->cancel_btn);

    /* Initially hidden */
    gtk_widget_set_visible (GTK_WIDGET (self), FALSE);
}

VarProgressOverlay *
var_progress_overlay_new (void)
{
    return g_object_new (VAR_TYPE_PROGRESS_OVERLAY, NULL);
}

void
var_progress_overlay_show (VarProgressOverlay *self, const char *title)
{
    g_return_if_fail (VAR_IS_PROGRESS_OVERLAY (self));
    gtk_label_set_text (GTK_LABEL (self->title_label), title ? title : "Working...");
    gtk_label_set_text (GTK_LABEL (self->detail_label), "");
    gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (self->progress_bar), 0.0);
    gtk_widget_set_sensitive (self->cancel_btn, TRUE);
    gtk_widget_set_visible (GTK_WIDGET (self), TRUE);
}

void
var_progress_overlay_update (VarProgressOverlay *self, double fraction, const char *detail)
{
    g_return_if_fail (VAR_IS_PROGRESS_OVERLAY (self));
    gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (self->progress_bar), fraction);
    
    char pct[16];
    g_snprintf (pct, sizeof(pct), "%.0f%%", fraction * 100.0);
    gtk_progress_bar_set_text (GTK_PROGRESS_BAR (self->progress_bar), pct);
    
    if (detail)
        gtk_label_set_text (GTK_LABEL (self->detail_label), detail);
}

void
var_progress_overlay_set_fraction (VarProgressOverlay *self, double fraction)
{
    g_return_if_fail (VAR_IS_PROGRESS_OVERLAY (self));
    gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (self->progress_bar), fraction);
    
    char pct[16];
    g_snprintf (pct, sizeof(pct), "%.0f%%", fraction * 100.0);
    gtk_progress_bar_set_text (GTK_PROGRESS_BAR (self->progress_bar), pct);
}

void
var_progress_overlay_set_detail (VarProgressOverlay *self, const char *detail)
{
    g_return_if_fail (VAR_IS_PROGRESS_OVERLAY (self));
    gtk_label_set_text (GTK_LABEL (self->detail_label), detail ? detail : "");
}

void
var_progress_overlay_hide (VarProgressOverlay *self)
{
    g_return_if_fail (VAR_IS_PROGRESS_OVERLAY (self));
    gtk_widget_set_visible (GTK_WIDGET (self), FALSE);
}

void
var_progress_overlay_set_cancel_cb (VarProgressOverlay *self, GCallback cb, gpointer user_data)
{
    g_return_if_fail (VAR_IS_PROGRESS_OVERLAY (self));
    self->cancel_cb = cb;
    self->cancel_data = user_data;
}
