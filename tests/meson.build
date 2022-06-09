test_proto_h_cc = custom_target(
  'test_proto_h_cc',
  output: [ 'testproto.pb.h', 'testproto.cc' ],
  input: [ 'testproto.proto', '../lib/common.proto' ],
  command: [ protoc, '--proto_path=@CURRENT_SOURCE_DIR@:@CURRENT_SOURCE_DIR@/..', '--cpp_out=@OUTDIR@', '@INPUT@' ]
)

test_proto_lib = static_library(
    'testproto',
    sources: [
      test_proto_h_cc
    ]
)

test_proto_dep = declare_dependency(
    include_directories: [
      test_proto_lib.private_dir_include()
    ],
    link_with: [
      test_proto_lib
    ]
)

TESTS = {
  'agg': [ agg_dep ],
  'amiga': [],
  'bitaccumulator': [],
  'bytes': [],
  'compression': [],
  'csvreader': [],
  'flags': [],
  'fluxmapreader': [],
  'fluxpattern': [],
  'fmmfm': [],
  'greaseweazle': [],
  'ldbs': [],
  'proto': [ test_proto_dep, config_proto_dep ],
  'utils': [],
}

foreach t, deps : TESTS

  executable(
    t,
    sources: [
      t + '.cc'
    ],
    dependencies: [
      libfluxengine_dep,
      fmt_dep,
      zlib_dep,
      protobuf_dep,
      snowhouse_dep,
      deps,
    ]
  )

endforeach
