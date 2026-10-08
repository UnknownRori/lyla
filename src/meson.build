wallpaper_engine_src = files(
  'main.c',
  './input.c',
  './text.c',
  './window_drag.c',
  './music/player.c',
  './visualizer/visualizer.c',
  './ui/common.c',
  './ui/visualizer.c',
  './ui/track_info.c',
  './ui/background.c',
  './ui/overlay.c',
)

src_files = files(
  'main.c',
  './lyla.c',
  './text.c',
  './input.c',
  './resources.c',
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
  './ui/confirm.c',
  './music/player.c',
  './music/track.c',
  './music/playlist.c',
  './music/playlist_search.c',
  './music/randomizer.c',
  './music/tag.c',
  './utf8/fold.c',
  './platform/tray.c',
  './platform/wallpaper.c',
  './platform/force.c',
  './thirdparty/discord.c',
  './thirdparty/qrcodegen.c',
  './thirdparty/profiler.c',
)
exe_args = {}
link_args = []
c_args = []

if os_is_emscripten
  c_args += '-DPLATFORM_WEB'
  c_args += '-DGRAPHICS_API_OPENGL_ES2'
  link_args += '-sUSE_GLFW=3'
  link_args += '-sASYNCIFY'
  link_args += '-sWASM=1'
  link_args += '-sALLOW_MEMORY_GROWTH=1'
  link_args += '-sINITIAL_MEMORY=67108864'

  # TODO : Put this on debug mode
  link_args += ['-sASSERTIONS=2', '-sSTACK_OVERFLOW_CHECK=2']
  # link_args += [
  #   '-gsource-map',
  #   '--source-map-base', 'http://127.0.0.1:8000/'
  # ]
endif

if get_option('WALLPAPER_ENGINE') and os_is_emscripten
  c_args += '-DWALLPAPER_ENGINE'
    link_args += [
      '-sEXPORTED_FUNCTIONS=["_main","_UpdateAudioData","_UpdateMediaProperties","_UpdateAlbumArt","_malloc","_free"]',
      '-sEXPORTED_RUNTIME_METHODS=["ccall","FS","stringToUTF8","lengthBytesUTF8"]'
    ]
  src_files = wallpaper_engine_src
endif


if os_windows
  winmod = import('windows')
  src_files += winmod.compile_resources('./app.rc')
  link_args += '-static'
  if get_option('buildtype') == 'release' or get_option('WINDOW_NO_CONSOLE')
    exe_args += {'win_subsystem': 'windows'}
  endif
endif

cc = meson.get_compiler('c')
m_dep = cc.find_library('m', required: false)

dependencies = [m_dep, raylib_dep, rconfig_dep]
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

if not get_option('WALLPAPER_ENGINE')
  dependencies += [taglib_dep, taglib_c_dep]
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
