project('walkie talkie', 'cpp', default_options: ['cpp_std=c++23'])

compiler = meson.get_compiler('cpp')

boost_deps = [
  dependency('boost')
]

libav_deps = [
  dependency('libavcodec'),
  dependency('libavformat'),
  dependency('libavdevice'),
  dependency('libavutil'),
  dependency('libswresample')
]

warning_flags = [
  '-Wall', '-Wformat', '-Wformat=2', 
  '-Wconversion', '-Wimplicit-fallthrough',
  '-Werror=format-security',
]

hardening_flags = [
  '-U_FORTIFY_SOURCE', '-D_FORTIFY_SOURCE=3',
  '-D_GLIBCXX_ASSERTIONS',
  '-fstrict-flex-arrays=3',
  '-fstack-clash-protection', 
  '-fstack-protector-strong',
]

project_compile_flags = ['-O2'] + warning_flags  + hardening_flags

linker_hardening_flags = [
  '-Wl,-z,nodlopen',
  '-Wl,-z,noexecstack',
  '-Wl,-z,relro',
  '-Wl,-z,now',
  '-Wl,--no-copy-dt-needed-entries',
]

project_linker_flags = linker_hardening_flags

add_project_arguments(
  compiler.get_supported_arguments(project_compile_flags),
  language: 'cpp'
)
 
add_project_link_arguments(
  compiler.get_supported_link_arguments(project_linker_flags),
  language: 'cpp'
)

files = [
  'src/main.cpp',
  'src/audio_input.cpp',
]

executable('yap', files, dependencies: boost_deps + libav_deps)