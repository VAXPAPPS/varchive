# VArchive
# Copyright (C) 2026 VAXP Organization

project('varchive', 'c',
        version: '1.0.0',
        meson_version: '>= 0.59.0',
        default_options: [
            'warning_level=2',
            'c_std=gnu17',
        ])

# Dependencies
gtk_dep = dependency('gtk4', version: '>= 4.6')
adwaita_dep = dependency('libadwaita-1', version: '>= 1.2')
libarchive_dep = dependency('libarchive', version: '>= 3.4')

# GNOME module for desktop/resources
gnome = import('gnome')

# Subdirectories
subdir('data')
subdir('src')
