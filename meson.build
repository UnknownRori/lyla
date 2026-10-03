project('lyla', 'c', default_options: [
    'c_std=c99'
])
add_project_arguments('-D_GNU_SOURCE', language: 'c')

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

if not get_option('NO_DISCORD')
  discord_opt = cmake.subproject_options()
  discord_opt.add_cmake_defines({
    'BUILD_TESTING':     false,
    'BUILD_EXAMPLES':    false,
    # 'CMAKE_CXX_FLAGS': '-fpermissive -Wno-class-memaccess -Wno-error=template-body'
  })

  discord = cmake.subproject('discord', options: discord_opt)
  discord_dep = discord.dependency('discord-rpc')
endif

taglib_opt = cmake.subproject_options()
taglib_opt.add_cmake_defines({
  'BUILD_SHARED_LIBS': false,
  'BUILD_TESTING':     false,
  'BUILD_EXAMPLES':    false,
  'WITH_ZLIB':         false,
})
taglib = cmake.subproject('taglib', options: taglib_opt)
taglib_dep = taglib.dependency('tag_c')
taglib_c_inc = include_directories('./subprojects/taglib/bindings/c')

subdir('src')
