/*
 * VArchive - Professional Archive Manager for VAXP-OS
 * Copyright (C) 2026 VAXP Organization
 *
 * var-toolbar.c — Main action toolbar widget
 */

#include "var-toolbar.h"

struct _VarToolbar {
    GtkBox       parent_instance;

    GtkWidget   *btn_open;
    GtkWidget   *btn_extract;
    GtkWidget   *btn_create;
    GtkWidget   *btn_add;
    GtkWidget   *btn_delete;
    GtkWidget   *btn_test;
    GtkWidget   *btn_properties;
    GtkWidget   *search_entry;
};

G_DEFINE_TYPE (VarToolbar, var_toolbar, GTK_TYPE_BOX)

enum {
    SIGNAL_ACTION_OPEN,
    SIGNAL_ACTION_EXTRACT,
    SIGNAL_ACTION_CREATE,
    SIGNAL_ACTION_ADD,
    SIGNAL_ACTION_DELETE,
    SIGNAL_ACTION_TEST,
    SIGNAL_ACTION_PROPERTIES,
    SIGNAL_SEARCH_CHANGED,
    N_SIGNALS
};

static guint signals[N_SIGNALS];

static void
on_btn_clicked (GtkButton *btn, gpointer user_data)
{
    VarToolbar *self = VAR_TOOLBAR (user_data);

    if (GTK_WIDGET (btn) == self->btn_open)
        g_signal_emit (self, signals[SIGNAL_ACTION_OPEN], 0);
    else if (GTK_WIDGET (btn) == self->btn_extract)
        g_signal_emit (self, signals[SIGNAL_ACTION_EXTRACT], 0);
    else if (GTK_WIDGET (btn) == self->btn_create)
        g_signal_emit (self, signals[SIGNAL_ACTION_CREATE], 0);
    else if (GTK_WIDGET (btn) == self->btn_add)
        g_signal_emit (self, signals[SIGNAL_ACTION_ADD], 0);
    else if (GTK_WIDGET (btn) == self->btn_delete)
        g_signal_emit (self, signals[SIGNAL_ACTION_DELETE], 0);
    else if (GTK_WIDGET (btn) == self->btn_test)
        g_signal_emit (self, signals[SIGNAL_ACTION_TEST], 0);
    else if (GTK_WIDGET (btn) == self->btn_properties)
        g_signal_emit (self, signals[SIGNAL_ACTION_PROPERTIES], 0);
}

static void
on_search_changed (GtkSearchEntry *entry, gpointer user_data)
{
    VarToolbar *self = VAR_TOOLBAR (user_data);
    const char *text = gtk_editable_get_text (GTK_EDITABLE (entry));
    g_signal_emit (self, signals[SIGNAL_SEARCH_CHANGED], 0, text);
}

static GtkWidget *
create_tool_button (const char *icon_name, const char *tooltip, VarToolbar *self)
{
    GtkWidget *btn = gtk_button_new_from_icon_name (icon_name);
    gtk_widget_set_tooltip_text (btn, tooltip);
    gtk_widget_add_css_class (btn, "var-tool-btn");
    g_signal_connect (btn, "clicked", G_CALLBACK (on_btn_clicked), self);
    return btn;
}

static void
var_toolbar_class_init (VarToolbarClass *klass)
{
    GObjectClass *obj_class = G_OBJECT_CLASS (klass);

    signals[SIGNAL_ACTION_OPEN]       = g_signal_new ("action-open",       G_TYPE_FROM_CLASS(klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL, G_TYPE_NONE, 0);
    signals[SIGNAL_ACTION_EXTRACT]    = g_signal_new ("action-extract",    G_TYPE_FROM_CLASS(klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL, G_TYPE_NONE, 0);
    signals[SIGNAL_ACTION_CREATE]     = g_signal_new ("action-create",     G_TYPE_FROM_CLASS(klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL, G_TYPE_NONE, 0);
    signals[SIGNAL_ACTION_ADD]        = g_signal_new ("action-add",        G_TYPE_FROM_CLASS(klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL, G_TYPE_NONE, 0);
    signals[SIGNAL_ACTION_DELETE]     = g_signal_new ("action-delete",     G_TYPE_FROM_CLASS(klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL, G_TYPE_NONE, 0);
    signals[SIGNAL_ACTION_TEST]       = g_signal_new ("action-test",       G_TYPE_FROM_CLASS(klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL, G_TYPE_NONE, 0);
    signals[SIGNAL_ACTION_PROPERTIES] = g_signal_new ("action-properties", G_TYPE_FROM_CLASS(klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL, G_TYPE_NONE, 0);
    signals[SIGNAL_SEARCH_CHANGED]    = g_signal_new ("search-changed",    G_TYPE_FROM_CLASS(klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL, G_TYPE_NONE, 1, G_TYPE_STRING);
}

static void
var_toolbar_init (VarToolbar *self)
{
    gtk_orientable_set_orientation (GTK_ORIENTABLE (self), GTK_ORIENTATION_HORIZONTAL);
    gtk_box_set_spacing (GTK_BOX (self), 6);
    gtk_widget_add_css_class (GTK_WIDGET (self), "var-toolbar");
    gtk_widget_set_margin_start (GTK_WIDGET (self), 8);
    gtk_widget_set_margin_end (GTK_WIDGET (self), 8);
    gtk_widget_set_margin_top (GTK_WIDGET (self), 4);
    gtk_widget_set_margin_bottom (GTK_WIDGET (self), 4);

    /* Primary buttons */
    self->btn_open    = create_tool_button ("document-open-symbolic",    "Open Archive (Ctrl+O)", self);
    self->btn_extract = create_tool_button ("archive-extract-symbolic", "Extract",             self);
    self->btn_create  = create_tool_button ("document-new-symbolic",     "Create Archive",      self);
    gtk_widget_add_css_class (self->btn_extract, "suggested-action");

    /* Secondary buttons */
    self->btn_add        = create_tool_button ("list-add-symbolic",           "Add Files",          self);
    self->btn_delete     = create_tool_button ("list-remove-symbolic",        "Delete Selected",    self);
    self->btn_test       = create_tool_button ("system-run-symbolic",         "Test Integrity",     self);
    self->btn_properties = create_tool_button ("document-properties-symbolic","Archive Properties", self);

    /* Search */
    self->search_entry = gtk_search_entry_new ();
    gtk_widget_set_hexpand (self->search_entry, TRUE);
    gtk_widget_set_halign (self->search_entry, GTK_ALIGN_END);
    g_signal_connect (self->search_entry, "search-changed", G_CALLBACK (on_search_changed), self);

    /* Separators */
    GtkWidget *sep1 = gtk_separator_new (GTK_ORIENTATION_VERTICAL);
    GtkWidget *sep2 = gtk_separator_new (GTK_ORIENTATION_VERTICAL);

    /* Layout */
    gtk_box_append (GTK_BOX (self), self->btn_open);
    gtk_box_append (GTK_BOX (self), self->btn_extract);
    gtk_box_append (GTK_BOX (self), self->btn_create);
    gtk_box_append (GTK_BOX (self), sep1);
    gtk_box_append (GTK_BOX (self), self->btn_add);
    gtk_box_append (GTK_BOX (self), self->btn_delete);
    gtk_box_append (GTK_BOX (self), sep2);
    gtk_box_append (GTK_BOX (self), self->btn_test);
    gtk_box_append (GTK_BOX (self), self->btn_properties);

    /* Spacer between buttons and search */
    GtkWidget *spacer = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_hexpand (spacer, TRUE);
    gtk_box_append (GTK_BOX (self), spacer);
    gtk_box_append (GTK_BOX (self), self->search_entry);

    var_toolbar_set_archive_opened (self, FALSE);
}

VarToolbar *
var_toolbar_new (void)
{
    return g_object_new (VAR_TYPE_TOOLBAR, NULL);
}

void
var_toolbar_set_archive_opened (VarToolbar *self, gboolean opened)
{
    g_return_if_fail (VAR_IS_TOOLBAR (self));
    gtk_widget_set_sensitive (self->btn_extract, opened);
    gtk_widget_set_sensitive (self->btn_add, opened);
    gtk_widget_set_sensitive (self->btn_test, opened);
    gtk_widget_set_sensitive (self->btn_properties, opened);
    gtk_widget_set_sensitive (self->search_entry, opened);
    var_toolbar_set_selection_count (self, 0);

    if (!opened) {
        gtk_editable_set_text (GTK_EDITABLE (self->search_entry), "");
    }
}

void
var_toolbar_set_selection_count (VarToolbar *self, int count)
{
    g_return_if_fail (VAR_IS_TOOLBAR (self));
    gtk_widget_set_sensitive (self->btn_delete, count > 0);
}

void
var_toolbar_set_search_active (VarToolbar *self, gboolean active)
{
    g_return_if_fail (VAR_IS_TOOLBAR (self));
    if (active)
        gtk_widget_grab_focus (self->search_entry);
    else
        gtk_editable_set_text (GTK_EDITABLE (self->search_entry), "");
}

const char *
var_toolbar_get_search_text (VarToolbar *self)
{
    g_return_val_if_fail (VAR_IS_TOOLBAR (self), NULL);
    return gtk_editable_get_text (GTK_EDITABLE (self->search_entry));
}
