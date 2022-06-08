subdir('formats')
subdir('gui')

executable(
  'fluxengine',
  sources: [
    'fe-analysedriveresponse.cc',
    'fe-analyselayout.cc',
    'fe-inspect.cc',
    'fe-rawread.cc',
    'fe-rawwrite.cc',
    'fe-read.cc',
    'fe-rpm.cc',
    'fe-seek.cc',
    'fe-testbandwidth.cc',
    'fe-testvoltages.cc',
    'fe-write.cc',
    'fluxengine.cc',
  ],
  dependencies: [
    agg_dep,
    config_proto_dep,
    formats_dep,
    fmt_dep,
    libfluxengine_dep,
    libusbp_dep,
    protobuf_dep,
    zlib_dep,
  ],
)


