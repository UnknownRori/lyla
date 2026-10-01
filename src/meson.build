src_files = files(
  'main.c',
  './lyla.c',
  './text.c',
  './input.c',
  './window_drag.c',
  './visualizer/fft.c',
  './visualizer/visualizer.c',
  './ui/common.c',
  './ui/notification.c',
  './ui/playlist.c',
  './ui/visualizer.c',
  './ui/timeline.c',
  './ui/track_info.c',
  './ui/background.c',
  './ui/overlay.c',
  './music/player.c',
  './music/track.c',
  './music/playlist.c',
  './music/randomizer.c',
  './music/tag.c',
  './thirdparty/discord.c',
  './thirdparty/qrcodegen.c',
  './resources.c',
  './platform/tray.c',
  './platform/wallpaper.c',
  './platform/force.c',
)
if host_machine.system() == 'windows'
  winmod = import('windows')
  src_files += winmod.compile_resources('./app.rc')
endif

exe_args = {}

if host_machine.system() == 'windows' and (get_option('buildtype') == 'release' or get_option('WINDOW_NO_CONSOLE'))
  exe_args += {'win_subsystem': 'windows'}
endif

cc = meson.get_compiler('c')
m_dep = cc.find_library('m', required: false)

dependencies = [m_dep, raylib_dep, rconfig_dep, discord_dep, taglib_dep]

executable(
  meson.project_name(),
  src_files,
  include_directories: [rstb_inc],
  dependencies: dependencies,
  link_args: ['-static'],
  kwargs: exe_args,
)
