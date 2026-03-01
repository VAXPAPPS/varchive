# VArchive - Professional Archive Manager for VAXP-OS
# Copyright (C) 2026 VAXP Organization
#
# src/meson.build — Source files definition

varchive_sources = [
    'main.c',

    # Core
    'core/var-utils.c',
    'core/var-application.c',

    # Backend
    'backend/var-backend.c',
    'backend/var-entry.c',
    'backend/var-libarchive-backend.c',
    'backend/var-command-backend.c',
    'backend/var-format-registry.c',

    # Services
    'services/var-archive-service.c',

    # Presentation (Widgets)
    'presentation/widgets/var-headerbar.c',
    'presentation/widgets/var-location-bar.c',
    'presentation/widgets/var-toolbar.c',
    'presentation/widgets/var-progress-overlay.c',
    'presentation/widgets/var-archive-view.c',

    # Presentation (Windows)
    'presentation/windows/var-main-window.c',

    # Presentation (Dialogs)
    'presentation/dialogs/var-extract-dialog.c',
    'presentation/dialogs/var-create-dialog.c',
    'presentation/dialogs/var-properties-dialog.c',
    'presentation/dialogs/var-preferences-dialog.c',
    'presentation/dialogs/var-password-dialog.c',
]

if get_option('buildtype') == 'debug'
    add_project_arguments('-DVAR_DEBUG=1', language: 'c')
endif

executable('varchive',
           varchive_sources,
           resources,
           dependencies: [
               gtk_dep,
               adwaita_dep,
               libarchive_dep,
           ],
           install: true)
