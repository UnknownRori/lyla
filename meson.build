project('lyla', 'c', default_options: [
    'c_std=c99'
])

rstb_inc = include_directories('./external/rstb')

raylib_proj = subproject('raylib', default_options: [
  'default_library=static',
  'SUPPORT_FILEFORMAT_FLAC=true',
  'SUPPORT_FILEFORMAT_MP3=true',
])
raylib_dep = raylib_proj.get_variable('raylib_dep')

rconfig_proj = subproject('rconfig', default_options: ['default_library=static'])
rconfig_dep = rconfig_proj.get_variable('rconfig_dep')

cmake   = import('cmake')
discord_opt = cmake.subproject_options()

discord = cmake.subproject('discord', options: discord_opt)
discord_dep = discord.dependency('discord-rpc')

subdir('src')
