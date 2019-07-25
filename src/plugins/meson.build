plugin_cpp_args = ['-DHAVE_CONFIG_H']

yolov2_sources = [
  'gsttinyyolov2.h',
  'gsttinyyolov2.cc'
]

yolov2 = shared_library('gsttinyyolov2',
  yolov2_sources,
  cpp_args: plugin_cpp_args,
  dependencies : [gst_dep, gstvideo_dep, lib_common, lib_fastoml],
  include_directories : [inc_common, inc_fastoml, public_incdir],
  install : true,
  install_dir : plugins_install_dir,
  link_with : backends
)

yolov3_sources = [
  'gsttinyyolov3.h',
  'gsttinyyolov3.cc'
]

yolov3 = shared_library('gsttinyyolov3',
  yolov3_sources,
  cpp_args: plugin_cpp_args,
  dependencies : [gst_dep, gstvideo_dep, lib_common, lib_fastoml],
  include_directories : [inc_common, inc_fastoml, public_incdir],
  install : true,
  install_dir : plugins_install_dir,
  link_with : backends
)

opencv_dep = dependency('opencv', version : ['>= 3.0.0'], required : false)
opencv_found = opencv_dep.found()
if not opencv_found
  opencv_dep = dependency('opencv4', required : false)
endif

opencv_found = opencv_dep.found()
if opencv_found
  detectionoverlay_sources = [
    'gstdetectionoverlay.h',
    'gstdetectionoverlay.cc'
  ]

  detectionoverlay = shared_library('gstdetectionoverlay',
    detectionoverlay_sources,
    cpp_args: plugin_cpp_args,
    dependencies : [gst_dep, gstvideo_dep, lib_common, lib_fastoml, opencv_dep],
    include_directories : [inc_common, inc_fastoml, public_incdir],
    install : true,
    install_dir : plugins_install_dir,
    link_with : backends
  )
endif
