project(
  'tbo',
  'c',
  version: '2.0',
  default_options: ['c_std=gnu11']
)

subdir('po')

cc = meson.get_compiler('c')

gtk_dep = dependency('gtk4', version: '>= 4.22')
cairo_dep = dependency('cairo')
rsvg_dep = dependency('librsvg-2.0')
math_dep = cc.find_library('m', required: false)

deps = [gtk_dep, cairo_dep, rsvg_dep, math_dep]

data_dir = join_paths(get_option('prefix'), get_option('datadir'), 'tbo')
locale_dir = join_paths(get_option('prefix'), get_option('localedir'))

conf = configuration_data()
conf.set_quoted('GETTEXT_PACKAGE', 'tbo')
conf.set_quoted('VERSION', meson.project_version())
conf.set10('ENABLE_NLS', true)
configure_file(output: 'config.h', configuration: conf)
configured_icon_png = configure_file(input: 'data/icon.png', output: 'tbo.png', copy: true)

add_project_arguments(
  '-DG_LOG_DOMAIN="tbo"',
  '-DGNOMELOCALEDIR="' + locale_dir + '"',
  '-DDATA_DIR="' + data_dir + '"',
  '-DSOURCE_DATA_DIR="' + join_paths(meson.project_source_root(), 'data') + '"',
  language: 'c'
)

inc = include_directories('src')

common_sources = files(
  'src/tbo-window.c',
  'src/tbo-file-dialog.c',
  'src/tbo-widget.c',
  'src/comic.c',
  'src/comic-new-dialog.c',
  'src/comic-saveas-dialog.c',
  'src/comic-open-dialog.c',
  'src/frame.c',
  'src/page.c',
  'src/ui-menu.c',
  'src/doodle-treeview.c',
  'src/tbo-tooltip.c',
  'src/export.c',
  'src/dnd.c',
  'src/comic-load.c',
  'src/tbo-utils.c',
  'src/tbo-files.c',
  'src/tbo-ui-utils.c',
  'src/tbo-object-base.c',
  'src/tbo-object-svg.c',
  'src/tbo-object-text.c',
  'src/tbo-object-pixmap.c',
  'src/tbo-object-group.c',
  'src/tbo-tool-base.c',
  'src/tbo-tool-selector.c',
  'src/tbo-tool-frame.c',
  'src/tbo-tool-doodle.c',
  'src/tbo-tool-bubble.c',
  'src/tbo-tool-text.c',
  'src/tbo-toolbar.c',
  'src/tbo-drawing.c',
  'src/tbo-undo.c'
)

executable(
  'tbo',
  common_sources + files('src/tbo.c'),
  dependencies: deps,
  include_directories: inc,
  install: true
)

load_render_check = executable(
  'load-render-check',
  common_sources + files('tests/load_render_check.c'),
  dependencies: deps,
  include_directories: inc,
  install: false
)

frame_tool_check = executable(
  'frame-tool-check',
  common_sources + files('tests/frame_tool_check.c'),
  dependencies: deps,
  include_directories: inc,
  install: false
)

save_roundtrip_check = executable(
  'save-roundtrip-check',
  common_sources + files('tests/save_roundtrip_check.c'),
  dependencies: deps,
  include_directories: inc,
  install: false
)

asset_bounds_check = executable(
  'asset-bounds-check',
  common_sources + files('tests/asset_bounds_check.c'),
  dependencies: deps,
  include_directories: inc,
  install: false
)

test(
  'load-render-check',
  load_render_check,
  args: [join_paths(meson.project_source_root(), 'data', 'tut.tbo')],
  env: ['G_DEBUG=fatal-criticals']
)

test(
  'frame-tool-check',
  frame_tool_check,
  args: [join_paths(meson.project_source_root(), 'data', 'tut.tbo')],
  env: ['G_DEBUG=fatal-criticals']
)

test(
  'save-roundtrip-check',
  save_roundtrip_check,
  args: [join_paths(meson.project_source_root(), 'data', 'tut.tbo')],
  env: ['G_DEBUG=fatal-criticals']
)

test(
  'asset-bounds-check',
  asset_bounds_check,
  args: [join_paths(meson.project_source_root(), 'data', 'tut.tbo')],
  env: ['G_DEBUG=fatal-criticals']
)

install_data(
  'data/tutorial.pdf',
  'data/tut.tbo',
  'data/icon.png',
  install_dir: join_paths(get_option('datadir'), 'tbo')
)

install_subdir('data/ui', install_dir: join_paths(get_option('datadir'), 'tbo'))
install_subdir('data/icons', install_dir: join_paths(get_option('datadir'), 'tbo'))
install_subdir('data/doodle', install_dir: join_paths(get_option('datadir'), 'tbo'))

install_data('data/applications/net.danigm.tbo.desktop', install_dir: join_paths(get_option('datadir'), 'applications'))
install_data('data/tbo.svg', install_dir: join_paths(get_option('datadir'), 'pixmaps'))
install_data('data/tbo.svg', install_dir: join_paths(get_option('datadir'), 'icons', 'hicolor', 'scalable', 'apps'))
install_data(configured_icon_png, install_dir: join_paths(get_option('datadir'), 'icons', 'hicolor', '128x128', 'apps'))
