plugin_cpp_args = ['-DHAVE_CONFIG_H']

gstvideo_dep = dependency('gstreamer-video-1.0',
    fallback: ['gst-plugins-base', 'video_dep'])

lib_common = cc.find_library('common', dirs : ['/usr/local/lib'])
inc_common = include_directories('/usr/local/include')

lib_fastoml = cc.find_library('fastoml', dirs : ['/usr/local/lib'])
inc_fastoml = include_directories('/usr/local/include')

lib_tensorflow = cc.find_library('tensorflow', dirs : ['/usr/local/lib'])
inc_tensorflow = include_directories('/usr/local/include')

backends_sources = [
  'gstbackend.cc',
  'gsttensorflow.cc',
]

backends = shared_library('fasto_ml_backends',
  [backends_sources],
  cpp_args: plugin_cpp_args,
  dependencies : [gst_dep, gstvideo_dep, lib_common, lib_fastoml, lib_tensorflow],
  include_directories : [inc_fastoml, inc_common, inc_tensorflow, public_incdir],
  install : true,
  install_dir : plugins_install_dir,
)

yolov2_sources = [
  'gstyolov2.h',
  'gstyolov2.cc'
]

yolov2 = shared_library('yolov2',
  yolov2_sources,
  cpp_args: plugin_cpp_args,
  dependencies : [gst_dep, gstvideo_dep, lib_common, lib_fastoml, lib_tensorflow],
  include_directories : [inc_fastoml, inc_common, inc_tensorflow, public_incdir],
  install : true,
  install_dir : plugins_install_dir,
  link_with : backends
)
