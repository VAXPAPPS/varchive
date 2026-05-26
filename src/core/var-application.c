/*
 * VArchive - Professional Archive Manager for VAXP-OS
 * Copyright (C) 2026 VAXP Organization
 *
 * var-application.c — Application class implementation
 */

#include "var-application.h"
#include "../presentation/windows/var-main-window.h"
#include "theme_manager.h"

struct _VarApplication {
    AdwApplication  parent_instance;
};

G_DEFINE_TYPE (VarApplication, var_application, ADW_TYPE_APPLICATION)

/* ── Load CSS from GResource ────────────────────── */

static void
var_application_load_css (void)
{
    GtkCssProvider *provider = gtk_css_provider_new ();
    gtk_css_provider_load_from_resource (provider, "/org/vaxp/archive/css/style.css");
    gtk_style_context_add_provider_for_display (
        gdk_display_get_default (),
        GTK_STYLE_PROVIDER (provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
    );
}

/* ── Actions ────────────────────────────────────── */

static void
on_quit (GSimpleAction *action, GVariant *param, gpointer app)
{
    (void)action; (void)param;
    g_application_quit (G_APPLICATION (app));
}

static void
on_about (GSimpleAction *action, GVariant *param, gpointer app)
{
    (void)action; (void)param;

    GtkWindow *win = gtk_application_get_active_window (GTK_APPLICATION (app));

    const char *developers[] = { "VAXP Team", NULL };

    adw_show_about_dialog (GTK_WIDGET (win),
        "application-name", VAR_APP_NAME,
        "application-icon", VAR_APP_ID,
        "version",          VAR_APP_VERSION,
        "copyright",        "© 2026 VAXP Organization",
        "license-type",     GTK_LICENSE_GPL_3_0,
        "comments",         "Professional archive manager for VAXP-OS.\n"
                            "Open, create, and extract archives in 40+ formats.",
        "developers",       developers,
        "website",          "https://github.com/VAXPAPPS/VArchive",
        NULL);
}

static const GActionEntry app_actions[] = {
    { "quit",  on_quit,  NULL, NULL, NULL, {0,0,0} },
    { "about", on_about, NULL, NULL, NULL, {0,0,0} },
};

/* ── Lifecycle ──────────────────────────────────── */

static void
var_application_activate (GApplication *app)
{
    GtkWindow *win = gtk_application_get_active_window (GTK_APPLICATION (app));
    if (!win) {
        win = GTK_WINDOW (var_main_window_new (VAR_APPLICATION (app)));
    }
    gtk_window_present (win);
}

static void
var_application_open (GApplication  *app,
                      GFile        **files,
                      int            n_files,
                      const char    *hint)
{
    (void)hint;

    VarMainWindow *win = NULL;
    GtkWindow *active = gtk_application_get_active_window (GTK_APPLICATION (app));

    if (active && VAR_IS_MAIN_WINDOW (active)) {
        win = VAR_MAIN_WINDOW (active);
    } else {
        win = var_main_window_new (VAR_APPLICATION (app));
    }

    if (n_files > 0) {
        var_main_window_open_file (win, files[0]);
    }

    gtk_window_present (GTK_WINDOW (win));
}

static void
var_application_startup (GApplication *app)
{
    G_APPLICATION_CLASS (var_application_parent_class)->startup (app);
    var_application_load_css ();

    g_action_map_add_action_entries (G_ACTION_MAP (app),
                                    app_actions,
                                    G_N_ELEMENTS (app_actions),
                                    app);

    theme_manager_init();

    /* Keyboard shortcuts */
    const char *quit_accels[]  = { "<Control>q", NULL };
    const char *open_accels[]  = { "<Control>o", NULL };
    gtk_application_set_accels_for_action (GTK_APPLICATION (app), "app.quit",  quit_accels);
    gtk_application_set_accels_for_action (GTK_APPLICATION (app), "win.open",  open_accels);
}

/* ── Class / Instance Init ──────────────────────── */

static void
var_application_class_init (VarApplicationClass *klass)
{
    GApplicationClass *app_class = G_APPLICATION_CLASS (klass);
    app_class->activate = var_application_activate;
    app_class->open     = var_application_open;
    app_class->startup  = var_application_startup;
}

static void
var_application_init (VarApplication *self)
{
    (void)self;
}

/* ── Constructor ────────────────────────────────── */

VarApplication *
var_application_new (void)
{
    return g_object_new (VAR_TYPE_APPLICATION,
                         "application-id", VAR_APP_ID,
                         "flags",          G_APPLICATION_HANDLES_OPEN,
                         NULL);
}
