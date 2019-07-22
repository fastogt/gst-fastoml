plugin_cpp_args = ['-DHAVE_CONFIG_H']

yolov2_sources = [
  'gstyolov2.h',
  'gstyolov2.cc'
]

yolov2 = shared_library('gstyolov2',
  yolov2_sources,
  cpp_args: plugin_cpp_args,
  dependencies : [gst_dep, gstvideo_dep, lib_common, lib_fastoml],
  include_directories : [inc_common, inc_fastoml, public_incdir],
  install : true,
  install_dir : plugins_install_dir,
  link_with : backends
)
