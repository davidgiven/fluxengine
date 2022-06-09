wx_dep = dependency('wxwidgets', required: false)
if wx_dep.found()

  executable(
    'fluxengine-gui',
    sources: [
      'layout.cpp',
      'main.cc',
      'mainwindow.cc',
      'visualisation.cc',
    ],
    dependencies: [
      agg_dep,
      config_proto_dep,
      formats_dep,
      fmt_dep,
      libfluxengine_dep,
      libusbp_dep,
      protobuf_dep,
      wx_dep,
      zlib_dep,
    ],
  )

endif

