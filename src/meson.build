src_files = files(
  'main.c',
)
if host_machine.system() == 'windows'
  winmod = import('windows')
  src_files += winmod.compile_resources('./app.rc')
endif

exe_args = {}

if host_machine.system() == 'windows' and get_option('buildtype') == 'release'
  exe_args += {'win_subsystem': 'windows'}
endif

dependencies = [raylib_dep, rconfig_dep, discord_dep]

executable(
  meson.project_name(),
  src_files,
  include_directories: [rstb_inc],
  dependencies: dependencies,
  kwargs: exe_args,
)
