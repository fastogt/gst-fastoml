gstvideo_dep = dependency('gstreamer-video-1.0',
    fallback: ['gst-plugins-base', 'video_dep'])

lib_common = cc.find_library('common', dirs : ['/usr/local/lib'])
inc_common = include_directories('/usr/local/include')

lib_fastoml = cc.find_library('fastoml', dirs : ['/usr/local/lib'])
inc_fastoml = include_directories('/usr/local/include')

lib_tensorflow = cc.find_library('tensorflow', dirs : ['/usr/local/lib'])
inc_tensorflow = include_directories('/usr/local/include')

backends_sources = [
  'gstinferencepreprocess.cc',
  'gstbackend.cc',
  'gsttensorflow.cc',
  'gstmlmeta.cc',
  'gstvideomlfilter.cc'
]

backends = shared_library('fastoml_backends',
  [backends_sources],
  dependencies : [gst_dep, gstvideo_dep, lib_common, lib_fastoml, lib_tensorflow],
  include_directories : [inc_fastoml, inc_common, inc_tensorflow, public_incdir],
  install : true,
  install_dir : libs_install_dir,
)

subdir('plugins')
