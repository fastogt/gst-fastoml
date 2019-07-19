plugin_c_args = ['-DHAVE_CONFIG_H']

cdata = configuration_data()
cdata.set_quoted('PACKAGE_VERSION', gst_version)
cdata.set_quoted('PACKAGE', 'FastoGT ML')
cdata.set_quoted('GST_LICENSE', 'LGPL')
cdata.set_quoted('GST_API_VERSION', api_version)
cdata.set_quoted('GST_PACKAGE_NAME', 'FastoGT yolov2 plugin')
cdata.set_quoted('GST_PACKAGE_ORIGIN', 'https://gstreamer.freedesktop.org')
configure_file(output : 'config.h', configuration : cdata)

gstvideo_dep = dependency('gstreamer-video-1.0',
    fallback: ['gst-plugins-base', 'video_dep'])

yolov2_sources = [
  'gstyolov2.h',
  'gstyolov2.c',
  ]

yolov2 = library('yolov2',
  yolov2_sources,
  c_args: plugin_c_args,
  dependencies : [gst_dep, gstvideo_dep],
  install : true,
  install_dir : plugins_install_dir,
)
