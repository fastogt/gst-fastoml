cdata = configuration_data()
cdata.set_quoted('PACKAGE_VERSION', gst_version)
cdata.set_quoted('PACKAGE', 'FastoGT ML')
cdata.set_quoted('GST_LICENSE', 'LGPL')
cdata.set_quoted('GST_API_VERSION', api_version)
cdata.set_quoted('GST_PACKAGE_NAME', 'FastoGT ML plugins')
cdata.set_quoted('GST_PACKAGE_ORIGIN', 'https://fastogt.com')
configure_file(output : 'config.h', configuration : cdata, install: true, install_dir: 'fastoml/gst')

public_headers = [
  'fastoml/gst/gstbackend.h',
  'fastoml/gst/gsttensorflow.h',
]

install_headers(public_headers, subdir: 'fastoml/gst')
