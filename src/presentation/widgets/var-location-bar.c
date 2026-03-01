/*
 * VArchive - Professional Archive Manager for VAXP-OS
 * Copyright (C) 2026 VAXP Organization
 *
 * var-location-bar.c — Breadcrumb navigation bar
 */

#include "var-location-bar.h"
#include <string.h>

struct _VarLocationBar {
    GtkBox       parent_instance;
    char        *current_path;
    GtkWidget   *box;
};

G_DEFINE_TYPE (VarLocationBar, var_location_bar, GTK_TYPE_BOX)

enum {
    SIGNAL_PATH_CHANGED,
    N_SIGNALS
};

static guint signals[N_SIGNALS];

static void
on_breadcrumb_clicked (GtkButton *btn, gpointer user_data)
{
    VarLocationBar *self = VAR_LOCATION_BAR (user_data);
    const char *target = g_object_get_data (G_OBJECT (btn), "path");

    if (target && g_strcmp0 (target, self->current_path) != 0) {
        g_signal_emit (self, signals[SIGNAL_PATH_CHANGED], 0, target);
    }
}

static void
rebuild_breadcrumbs (VarLocationBar *self)
{
    /* Clear current box */
    GtkWidget *child;
    while ((child = gtk_widget_get_first_child (self->box)) != NULL)
        gtk_box_remove (GTK_BOX (self->box), child);

    /* Root button */
    GtkWidget *root_btn = gtk_button_new_from_icon_name ("go-home-symbolic");
    gtk_widget_add_css_class (root_btn, "var-breadcrumb-btn");
    gtk_widget_add_css_class (root_btn, "flat");
    g_object_set_data (G_OBJECT (root_btn), "path", (gpointer)"");
    g_signal_connect (root_btn, "clicked", G_CALLBACK (on_breadcrumb_clicked), self);
    gtk_box_append (GTK_BOX (self->box), root_btn);

    if (!self->current_path || self->current_path[0] == '\0')
        return;

    g_auto(GStrv) parts = g_strsplit (self->current_path, "/", -1);
    GString *accum = g_string_new ("");

    for (int i = 0; parts[i] && parts[i][0]; i++) {
        /* Separator arrow */
        GtkWidget *sep = gtk_image_new_from_icon_name ("pan-end-symbolic");
        gtk_widget_add_css_class (sep, "var-breadcrumb-sep");
        gtk_box_append (GTK_BOX (self->box), sep);

        /* Accumulate path */
        if (i > 0) g_string_append_c (accum, '/');
        g_string_append (accum, parts[i]);

        /* Button */
        GtkWidget *btn = gtk_button_new_with_label (parts[i]);
        gtk_widget_add_css_class (btn, "var-breadcrumb-btn");
        gtk_widget_add_css_class (btn, "flat");

        /* Store path in object data */
        g_object_set_data_full (G_OBJECT (btn), "path",
                                g_strdup (accum->str), g_free);
        g_signal_connect (btn, "clicked", G_CALLBACK (on_breadcrumb_clicked), self);

        gtk_box_append (GTK_BOX (self->box), btn);
    }

    g_string_free (accum, TRUE);
}

void
var_location_bar_set_path (VarLocationBar *self, const char *path)
{
    g_return_if_fail (VAR_IS_LOCATION_BAR (self));
    if (g_strcmp0 (self->current_path, path) == 0) return;

    g_free (self->current_path);
    self->current_path = g_strdup (path ? path : "");
    rebuild_breadcrumbs (self);
}

const char *
var_location_bar_get_path (VarLocationBar *self)
{
    g_return_val_if_fail (VAR_IS_LOCATION_BAR (self), NULL);
    return self->current_path;
}

static void
var_location_bar_finalize (GObject *obj)
{
    VarLocationBar *self = VAR_LOCATION_BAR (obj);
    g_free (self->current_path);
    G_OBJECT_CLASS (var_location_bar_parent_class)->finalize (obj);
}

static void
var_location_bar_class_init (VarLocationBarClass *klass)
{
    GObjectClass *obj_class = G_OBJECT_CLASS (klass);
    obj_class->finalize = var_location_bar_finalize;

    signals[SIGNAL_PATH_CHANGED] = g_signal_new (
        "path-changed",
        G_TYPE_FROM_CLASS (klass),
        G_SIGNAL_RUN_LAST,
        0, NULL, NULL, NULL,
        G_TYPE_NONE, 1,
        G_TYPE_STRING
    );
}

static void
var_location_bar_init (VarLocationBar *self)
{
    gtk_orientable_set_orientation (GTK_ORIENTABLE (self), GTK_ORIENTATION_HORIZONTAL);
    gtk_widget_add_css_class (GTK_WIDGET (self), "var-location-bar");

    self->current_path = g_strdup ("");

    /* Scrollable area for long paths */
    GtkWidget *scroll = gtk_scrolled_window_new ();
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll),
                                    GTK_POLICY_AUTOMATIC, GTK_POLICY_NEVER);
    gtk_widget_set_hexpand (scroll, TRUE);

    self->box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 4);
    gtk_widget_set_halign (self->box, GTK_ALIGN_START);
    gtk_widget_set_valign (self->box, GTK_ALIGN_CENTER);
    gtk_scrolled_window_set_child (GTK_SCROLLED_WINDOW (scroll), self->box);

    gtk_box_append (GTK_BOX (self), scroll);

    rebuild_breadcrumbs (self);
}

VarLocationBar *
var_location_bar_new (void)
{
    return g_object_new (VAR_TYPE_LOCATION_BAR, NULL);
}
