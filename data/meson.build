# Data dir meson.build
install_data('org.vaxp.archive.desktop',
  install_dir: get_option('datadir') / 'applications'
)

# Compile resources
resources = gnome.compile_resources('varchive-resources',
  'org.vaxp.archive.gresource.xml',
  c_name: 'varchive',
  source_dir: '.'
)

# Install icons
install_data('icons/org.vaxp.archive.svg',
  install_dir: get_option('datadir') / 'icons/hicolor/scalable/apps'
)
