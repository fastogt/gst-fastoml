project('gst-fastoml', 'cpp', default_options : ['c_std=c99', 'cpp_std=c++11'], version : '1.0.0.0', license : 'LGPL')

libs_install_dir = get_option('libdir')
plugins_install_dir = join_paths(libs_install_dir, 'gstreamer-1.0')

cc = meson.get_compiler('cpp')
add_global_arguments(['-DOS_POSIX', '-DCOMPILER_GCC'], language : 'cpp')
gst_version = meson.project_version()
api_version = '1.0'
gst_dep = dependency('gstreamer-1.0', fallback : ['gstreamer', 'gst_dep'])

public_incdir = include_directories('include')

subdir('include')
subdir('src')
