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
  './resources.c',
  './platform/tray.c',
  './platform/wallpaper.c',
  './platform/force.c',
  './thirdparty/discord.c',
  './thirdparty/qrcodegen.c',
  './thirdparty/profiler.c',
)
if host_machine.system() == 'windows'
  winmod = import('windows')
  src_files += winmod.compile_resources('./app.rc')
endif

exe_args = {}
link_args = []
c_args = []

if host_machine.system() == 'windows'  
  link_args += '-static'
  if get_option('buildtype') == 'release' or get_option('WINDOW_NO_CONSOLE')
    exe_args += {'win_subsystem': 'windows'}
  endif
endif

cc = meson.get_compiler('c')
m_dep = cc.find_library('m', required: false)

dependencies = [m_dep, raylib_dep, rconfig_dep, taglib_dep, taglib_c_dep]
if get_option('NO_DISCORD')
  c_args += '-DNO_DISCORD'
else
  dependencies += discord_dep
endif
if get_option('WITH_PROFILER')
  dependencies += iprof_dep
  c_args += '-DWITH_PROFILER'
endif

if get_option('WITH_MICROPHONE')
  c_args += '-DWITH_MICROPHONE'
endif


if host_machine.system() == 'linux'
  dependencies += [
    dependency('x11'),
    dependency('xext'),
    dependency('xrandr'),
    dependency('xcursor'),
    dependency('xfixes'),
    dependency('threads'),
  ]
endif

executable(
  meson.project_name(),
  src_files,
  include_directories: [rstb_inc, taglib_c_inc],
  dependencies: dependencies,
  link_args: link_args,
  c_args: c_args,
  kwargs: exe_args,
  install: true,
  install_dir: '.',
)

install_data('./icon.ico'     , install_dir: '.', install_tag: 'file' )
install_data('../README.md'   , install_dir: '.', install_tag: 'file' )
install_data('../LICENSE'     , install_dir: '.', install_tag: 'file' )
install_data('../CREDITS.txt' , install_dir: '.', install_tag: 'file' )
