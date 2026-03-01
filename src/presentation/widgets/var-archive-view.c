/*
 * VArchive - Professional Archive Manager for VAXP-OS
 * Copyright (C) 2026 VAXP Organization
 *
 * var-archive-view.c — Archive contents tree view
 */

#include "var-archive-view.h"
#include "../../core/var-utils.h"

struct _VarArchiveView {
    GtkBox           parent_instance;

    GtkWidget       *scrolled_window;
    GtkWidget       *column_view;

    GListStore      *root_model;
    GtkTreeListModel *tree_model;
    GtkSortListModel *sort_model;
    GtkFilterListModel *filter_model;
    GtkStringFilter *filter;
    GtkMultiSelection *selection;

    char            *current_folder;
    GPtrArray       *all_entries; /* flat list of all VarEntrys */
    
    GtkWidget       *context_menu;
};

G_DEFINE_TYPE (VarArchiveView, var_archive_view, GTK_TYPE_BOX)

enum {
    SIGNAL_FOLDER_ACTIVATED,
    SIGNAL_FILE_ACTIVATED,
    SIGNAL_SELECTION_CHANGED,
    N_SIGNALS
};

static guint signals[N_SIGNALS];

/* ═══════════════════════════════════════════════════
   Model setup
   ═══════════════════════════════════════════════════ */

static GListModel *
create_tree_model_func (gpointer item, gpointer user_data)
{
    (void)user_data;
    VarEntry *entry = VAR_ENTRY (item);
    
    if (var_entry_get_entry_type (entry) != VAR_ENTRY_DIRECTORY)
        return NULL;

    GPtrArray *children = var_entry_get_children (entry);
    if (!children || children->len == 0)
        return NULL;

    GListStore *store = g_list_store_new (VAR_TYPE_ENTRY);
    for (guint i = 0; i < children->len; i++) {
        g_list_store_append (store, g_ptr_array_index (children, i));
    }
    return G_LIST_MODEL (store);
}

static char *
get_node_path (GObject *item)
{
    if (GTK_IS_TREE_LIST_ROW (item)) {
        GObject *obj = gtk_tree_list_row_get_item (GTK_TREE_LIST_ROW (item));
        if (VAR_IS_ENTRY (obj)) {
            char *p = g_strdup (var_entry_get_path (VAR_ENTRY (obj)));
            g_object_unref (obj);
            return p;
        }
        g_clear_object (&obj);
    }
    return NULL;
}

static void
on_selection_changed (GtkSelectionModel *model, guint position, guint n_items, gpointer user_data)
{
    (void)position; (void)n_items;
    VarArchiveView *self = VAR_ARCHIVE_VIEW (user_data);
    
    GtkBitset *bitset = gtk_selection_model_get_selection (model);
    guint count = gtk_bitset_get_size (bitset);
    gtk_bitset_unref (bitset);
    
    g_signal_emit (self, signals[SIGNAL_SELECTION_CHANGED], 0, count);
}

static void
on_row_activated (GtkColumnView *view, guint pos, gpointer user_data)
{
    VarArchiveView *self = VAR_ARCHIVE_VIEW (user_data);
    GListModel *model = G_LIST_MODEL (gtk_column_view_get_model (view));
    g_autoptr(GObject) item = g_list_model_get_item (model, pos);
    
    if (!GTK_IS_TREE_LIST_ROW (item)) return;
    
    GtkTreeListRow *row = GTK_TREE_LIST_ROW (item);
    g_autoptr(VarEntry) entry = VAR_ENTRY (gtk_tree_list_row_get_item (row));
    
    if (!entry) return;

    if (var_entry_get_entry_type (entry) == VAR_ENTRY_DIRECTORY) {
        /* Toggle expansion if it has children, else navigate */
        if (gtk_tree_list_row_is_expandable (row)) {
            gtk_tree_list_row_set_expanded (row, !gtk_tree_list_row_get_expanded (row));
        }
        g_signal_emit (self, signals[SIGNAL_FOLDER_ACTIVATED], 0, var_entry_get_path (entry));
    } else {
        g_signal_emit (self, signals[SIGNAL_FILE_ACTIVATED], 0, var_entry_get_path (entry));
    }
}

static void
on_right_click (GtkGestureClick *gesture, int n_press, double x, double y, gpointer user_data)
{
    VarArchiveView *self = VAR_ARCHIVE_VIEW (user_data);
    if (n_press != 1) return;

    /* Get item at coordinates (roughly) - GTK4 makes this tricky with ColumnView,
       so we just show the menu at the pointer if there's a selection */
    
    if (var_archive_view_has_selection (self)) {
        GdkRectangle rect = { (int)x, (int)y, 1, 1 };
        gtk_popover_set_pointing_to (GTK_POPOVER (self->context_menu), &rect);
        gtk_popover_popup (GTK_POPOVER (self->context_menu));
    }
}

/* ═══════════════════════════════════════════════════
   Column Factories
   ═══════════════════════════════════════════════════ */

static VarEntry *
get_entry_from_list_item (GtkListItem *list_item)
{
    GObject *item = gtk_list_item_get_item (list_item);
    if (!item || !GTK_IS_TREE_LIST_ROW (item)) return NULL;
    GObject *data = gtk_tree_list_row_get_item (GTK_TREE_LIST_ROW (item));
    if (data) g_object_unref (data); /* get_item adds a ref */
    return VAR_ENTRY (data);
}

/* Name Column */
static void
setup_name_cb (GtkSignalListItemFactory *factory, GtkListItem *list_item, gpointer data)
{
    (void)factory; (void)data;
    GtkWidget *box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
    GtkWidget *icon = gtk_image_new ();
    GtkWidget *label = gtk_label_new ("");
    
    gtk_label_set_ellipsize (GTK_LABEL (label), PANGO_ELLIPSIZE_END);
    gtk_widget_set_hexpand (label, TRUE);
    gtk_widget_set_halign (label, GTK_ALIGN_START);
    
    /* Tree expander */
    GtkWidget *expander = gtk_tree_expander_new ();
    
    gtk_box_append (GTK_BOX (box), icon);
    gtk_box_append (GTK_BOX (box), label);
    gtk_tree_expander_set_child (GTK_TREE_EXPANDER (expander), box);
    
    gtk_list_item_set_child (list_item, expander);
}

static void
bind_name_cb (GtkSignalListItemFactory *factory, GtkListItem *list_item, gpointer data)
{
    (void)factory; (void)data;
    GtkWidget *expander = gtk_list_item_get_child (list_item);
    GtkTreeListRow *row = GTK_TREE_LIST_ROW (gtk_list_item_get_item (list_item));
    gtk_tree_expander_set_list_row (GTK_TREE_EXPANDER (expander), row);

    GtkWidget *box = gtk_tree_expander_get_child (GTK_TREE_EXPANDER (expander));
    GtkWidget *icon = gtk_widget_get_first_child (box);
    GtkWidget *label = gtk_widget_get_next_sibling (icon);

    VarEntry *entry = get_entry_from_list_item (list_item);
    if (!entry) return;

    /* Icon */
    if (var_entry_get_entry_type (entry) == VAR_ENTRY_DIRECTORY) {
        gtk_image_set_from_icon_name (GTK_IMAGE (icon), "folder-symbolic");
        gtk_widget_add_css_class (icon, "folder-icon");
    } else {
        const char *icon_name = var_utils_get_file_icon_name (var_entry_get_filename (entry));
        gtk_image_set_from_icon_name (GTK_IMAGE (icon), icon_name);
        gtk_widget_remove_css_class (icon, "folder-icon");
        gtk_widget_add_css_class (icon, "file-icon");
    }

    /* Label */
    gtk_label_set_text (GTK_LABEL (label), var_entry_get_filename (entry));
    
    /* Password indicator */
    if (var_entry_get_encrypted (entry)) {
        GtkWidget *lock = gtk_image_new_from_icon_name ("changes-prevent-symbolic");
        gtk_widget_set_margin_start (lock, 4);
        gtk_box_append (GTK_BOX (box), lock);
    }
}

/* Size Column */
static void
setup_size_cb (GtkSignalListItemFactory *factory, GtkListItem *list_item, gpointer data)
{
    (void)factory; (void)data;
    GtkWidget *label = gtk_label_new ("");
    gtk_widget_set_halign (label, GTK_ALIGN_END);
    gtk_list_item_set_child (list_item, label);
}

static void
bind_size_cb (GtkSignalListItemFactory *factory, GtkListItem *list_item, gpointer data)
{
    (void)factory; (void)data;
    GtkWidget *label = gtk_list_item_get_child (list_item);
    VarEntry *entry = get_entry_from_list_item (list_item);
    if (!entry) return;

    if (var_entry_get_entry_type (entry) == VAR_ENTRY_DIRECTORY) {
        gtk_label_set_text (GTK_LABEL (label), "--");
    } else {
        gint64 size = var_entry_get_size (entry);
        if (size >= 0) {
            g_autofree char *str = var_utils_format_size (size);
            gtk_label_set_text (GTK_LABEL (label), str);
        } else {
            gtk_label_set_text (GTK_LABEL (label), "Unknown");
        }
    }
}

/* Type Column */
static void
setup_type_cb (GtkSignalListItemFactory *factory, GtkListItem *list_item, gpointer data)
{
    (void)factory; (void)data;
    GtkWidget *label = gtk_label_new ("");
    gtk_widget_set_halign (label, GTK_ALIGN_START);
    gtk_label_set_ellipsize (GTK_LABEL (label), PANGO_ELLIPSIZE_END);
    gtk_list_item_set_child (list_item, label);
}

static void
bind_type_cb (GtkSignalListItemFactory *factory, GtkListItem *list_item, gpointer data)
{
    (void)factory; (void)data;
    GtkWidget *label = gtk_list_item_get_child (list_item);
    VarEntry *entry = get_entry_from_list_item (list_item);
    if (!entry) return;

    if (var_entry_get_entry_type (entry) == VAR_ENTRY_DIRECTORY) {
        gtk_label_set_text (GTK_LABEL (label), "Folder");
    } else {
        /* Basic type detection from extension */
        const char *filename = var_entry_get_filename (entry);
        const char *ext = strrchr (filename, '.');
        if (ext) {
            char *type = g_strdup_printf ("%s File", ext + 1);
            type[0] = g_ascii_toupper (type[0]);
            gtk_label_set_text (GTK_LABEL (label), type);
            g_free(type);
        } else {
            gtk_label_set_text (GTK_LABEL (label), "File");
        }
    }
}

/* Date Column */
static void
setup_date_cb (GtkSignalListItemFactory *factory, GtkListItem *list_item, gpointer data)
{
    (void)factory; (void)data;
    GtkWidget *label = gtk_label_new ("");
    gtk_widget_set_halign (label, GTK_ALIGN_END);
    gtk_list_item_set_child (list_item, label);
}

static void
bind_date_cb (GtkSignalListItemFactory *factory, GtkListItem *list_item, gpointer data)
{
    (void)factory; (void)data;
    GtkWidget *label = gtk_list_item_get_child (list_item);
    VarEntry *entry = get_entry_from_list_item (list_item);
    if (!entry) return;

    gint64 mtime = var_entry_get_mod_time (entry);
    if (mtime > 0) {
        g_autofree char *str = var_utils_format_date (mtime);
        gtk_label_set_text (GTK_LABEL (label), str);
    } else {
        gtk_label_set_text (GTK_LABEL (label), "--");
    }
}

/* ═══════════════════════════════════════════════════
   Initialization
   ═══════════════════════════════════════════════════ */

static void
var_archive_view_class_init (VarArchiveViewClass *klass)
{
    GObjectClass *obj_class = G_OBJECT_CLASS (klass);
    (void)obj_class;
    
    signals[SIGNAL_FOLDER_ACTIVATED] = g_signal_new ("folder-activated", G_TYPE_FROM_CLASS(klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL, G_TYPE_NONE, 1, G_TYPE_STRING);
    signals[SIGNAL_FILE_ACTIVATED]   = g_signal_new ("file-activated",   G_TYPE_FROM_CLASS(klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL, G_TYPE_NONE, 1, G_TYPE_STRING);
    signals[SIGNAL_SELECTION_CHANGED]= g_signal_new ("selection-changed",G_TYPE_FROM_CLASS(klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL, G_TYPE_NONE, 1, G_TYPE_INT);
}

static GtkColumnViewColumn *
create_column (const char *title, GCallback setup_cb, GCallback bind_cb, int width, gboolean resizable, gboolean expand)
{
    GtkListItemFactory *factory = gtk_signal_list_item_factory_new ();
    g_signal_connect (factory, "setup", setup_cb, NULL);
    g_signal_connect (factory, "bind", bind_cb, NULL);
    
    GtkColumnViewColumn *col = gtk_column_view_column_new (title, factory);
    gtk_column_view_column_set_resizable (col, resizable);
    if (width > 0) gtk_column_view_column_set_fixed_width (col, width);
    gtk_column_view_column_set_expand (col, expand);
    
    return col;
}

static void
var_archive_view_init (VarArchiveView *self)
{
    gtk_orientable_set_orientation (GTK_ORIENTABLE (self), GTK_ORIENTATION_VERTICAL);
    gtk_widget_set_vexpand (GTK_WIDGET (self), TRUE);
    gtk_widget_set_hexpand (GTK_WIDGET (self), TRUE);

    self->scrolled_window = gtk_scrolled_window_new ();
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (self->scrolled_window),
                                    GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_vexpand (self->scrolled_window, TRUE);
    gtk_widget_set_hexpand (self->scrolled_window, TRUE);
    gtk_widget_add_css_class (self->scrolled_window, "var-archive-view");
    gtk_box_append (GTK_BOX (self), self->scrolled_window);

    /* Models */
    self->root_model = g_list_store_new (VAR_TYPE_ENTRY);
    self->tree_model = gtk_tree_list_model_new (G_LIST_MODEL (self->root_model),
                                                FALSE, /* passthrough */
                                                TRUE,  /* autoexpand */
                                                create_tree_model_func,
                                                self, NULL);
    
    self->filter = gtk_string_filter_new (gtk_property_expression_new (VAR_TYPE_ENTRY, NULL, "filename"));
    gtk_string_filter_set_match_mode (self->filter, GTK_STRING_FILTER_MATCH_MODE_SUBSTRING);
    gtk_string_filter_set_ignore_case (self->filter, TRUE);

    /* Selection */
    self->selection = gtk_multi_selection_new (G_LIST_MODEL (self->tree_model));
    g_signal_connect (self->selection, "selection-changed", G_CALLBACK (on_selection_changed), self);

    /* Column View */
    self->column_view = gtk_column_view_new (GTK_SELECTION_MODEL (self->selection));
    gtk_widget_add_css_class (self->column_view, "data-table");
    gtk_column_view_set_show_row_separators (GTK_COLUMN_VIEW (self->column_view), TRUE);
    gtk_column_view_set_show_column_separators (GTK_COLUMN_VIEW (self->column_view), FALSE);
    g_signal_connect (self->column_view, "activate", G_CALLBACK (on_row_activated), self);

    /* Columns */
    gtk_column_view_append_column (GTK_COLUMN_VIEW (self->column_view),
                                   create_column ("Name", G_CALLBACK(setup_name_cb), G_CALLBACK(bind_name_cb), 250, TRUE, TRUE));
    gtk_column_view_append_column (GTK_COLUMN_VIEW (self->column_view),
                                   create_column ("Size", G_CALLBACK(setup_size_cb), G_CALLBACK(bind_size_cb), 100, TRUE, FALSE));
    gtk_column_view_append_column (GTK_COLUMN_VIEW (self->column_view),
                                   create_column ("Type", G_CALLBACK(setup_type_cb), G_CALLBACK(bind_type_cb), 120, TRUE, FALSE));
    gtk_column_view_append_column (GTK_COLUMN_VIEW (self->column_view),
                                   create_column ("Modified", G_CALLBACK(setup_date_cb), G_CALLBACK(bind_date_cb), 160, TRUE, FALSE));

    /* Context Menu */
    GMenu *menu = g_menu_new ();
    g_menu_append (menu, "Open", "win.open-selected");
    g_menu_append (menu, "Extract...", "win.extract-selected");
    g_menu_append (menu, "Delete", "win.delete-selected");
    g_menu_append (menu, "Properties", "win.properties-selected");
    
    self->context_menu = gtk_popover_menu_new_from_model (G_MENU_MODEL (menu));
    gtk_widget_set_parent (self->context_menu, GTK_WIDGET (self->column_view));
    gtk_popover_set_position (GTK_POPOVER (self->context_menu), GTK_POS_BOTTOM);
    gtk_popover_set_has_arrow (GTK_POPOVER (self->context_menu), FALSE);
    g_object_unref (menu);

    /* Right click gesture */
    GtkGesture *click = gtk_gesture_click_new ();
    gtk_gesture_single_set_button (GTK_GESTURE_SINGLE (click), GDK_BUTTON_SECONDARY);
    g_signal_connect (click, "pressed", G_CALLBACK (on_right_click), self);
    gtk_widget_add_controller (self->column_view, GTK_EVENT_CONTROLLER (click));

    gtk_scrolled_window_set_child (GTK_SCROLLED_WINDOW (self->scrolled_window), self->column_view);
}

VarArchiveView *
var_archive_view_new (void)
{
    return g_object_new (VAR_TYPE_ARCHIVE_VIEW, NULL);
}

/* ═══════════════════════════════════════════════════
   Public API
   ═══════════════════════════════════════════════════ */

void
var_archive_view_set_model (VarArchiveView *self, GPtrArray *entries)
{
    g_return_if_fail (VAR_IS_ARCHIVE_VIEW (self));
    
    var_archive_view_clear (self);
    
    if (self->all_entries)
        g_ptr_array_unref (self->all_entries);
    
    self->all_entries = entries ? g_ptr_array_ref (entries) : NULL;

    if (!entries) return;

    /* Build tree. For now, flat insert or simple root filter */
    /* If the entries array represents standard tree (has children setup), just add roots */
    for (guint i = 0; i < entries->len; i++) {
        VarEntry *e = g_ptr_array_index (entries, i);
        if (!var_entry_get_parent (e)) {
            g_list_store_append (self->root_model, e);
        }
    }
}

void
var_archive_view_clear (VarArchiveView *self)
{
    g_return_if_fail (VAR_IS_ARCHIVE_VIEW (self));
    g_list_store_remove_all (self->root_model);
}

void
var_archive_view_set_current_folder (VarArchiveView *self, const char *path)
{
    (void)self; (void)path;
    /* To implement: filter view by current folder path.
       With GtkTreeListModel it's complex to restrict purely to one subtree.
       Usually flat view filtering is preferred for folder nav. */
}

const char *
var_archive_view_get_current_folder (VarArchiveView *self)
{
    return self->current_folder;
}

GPtrArray *
var_archive_view_get_selected_paths (VarArchiveView *self)
{
    g_return_val_if_fail (VAR_IS_ARCHIVE_VIEW (self), NULL);

    GPtrArray *paths = g_ptr_array_new_with_free_func (g_free);
    GtkBitset *bitset = gtk_selection_model_get_selection (GTK_SELECTION_MODEL (self->selection));
    
    GtkBitsetIter iter;
    guint pos;
    gboolean valid = gtk_bitset_iter_init_first (&iter, bitset, &pos);
    
    while (valid) {
        g_autoptr(GObject) item = g_list_model_get_item (G_LIST_MODEL (self->selection), pos);
        char *path = get_node_path (item);
        if (path) g_ptr_array_add (paths, path);
        valid = gtk_bitset_iter_next (&iter, &pos);
    }
    
    gtk_bitset_unref (bitset);
    return paths;
}

void
var_archive_view_set_search_filter (VarArchiveView *self, const char *text)
{
    g_return_if_fail (VAR_IS_ARCHIVE_VIEW (self));
    if (text && text[0]) {
        gtk_string_filter_set_search (self->filter, text);
        /* Note: Needs FilterListModel wrapped around selection or root store 
           This is a simplified wire-up; fully correct GTK4 tree filtering requires custom filters */
    } else {
        gtk_string_filter_set_search (self->filter, NULL);
    }
}

gboolean
var_archive_view_has_selection (VarArchiveView *self)
{
    GtkBitset *bitset = gtk_selection_model_get_selection (GTK_SELECTION_MODEL (self->selection));
    gboolean empty = gtk_bitset_is_empty (bitset);
    gtk_bitset_unref (bitset);
    return !empty;
}
