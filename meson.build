project('fluxengine', 'cpp', 'c',
  default_options: ['cpp_std=c++2a']
)
add_global_arguments('-fext-numeric-literals', language: 'cpp')
add_global_arguments('-Wno-deprecated-enum-enum-conversion', language: 'cpp')
add_global_arguments('-Wno-deprecated-enum-float-conversion', language: 'cpp')

zlib_dep = dependency('zlib')
protobuf_dep = dependency('protobuf')
pthreads_dep = dependency('threads')
protoc = find_program('protoc', required : true)

protoc_generator = generator(
  protoc,
  output: ['@BASENAME@.pb.cc', '@BASENAME@.pb.h'],
  arguments: ['--proto_path=@CURRENT_SOURCE_DIR@', '--cpp_out=@BUILD_DIR@', '@INPUT@'],
)

subdir('dep/agg')
subdir('dep/fmt')
subdir('dep/libusbp')
subdir('dep/stb')

config_proto_h_cc = protoc_generator.process(
  'lib/decoders/decoders.proto',
  'lib/encoders/encoders.proto',
  'lib/fluxsink/fluxsink.proto',
  'lib/fluxsource/fluxsource.proto',
  'lib/imagereader/imagereader.proto',
  'lib/imagewriter/imagewriter.proto',
  'lib/usb/usb.proto',
  'lib/common.proto',
  'lib/config.proto',
  'lib/mapper.proto',
  'lib/drive.proto',
  'arch/aeslanier/aeslanier.proto',
  'arch/amiga/amiga.proto',
  'arch/apple2/apple2.proto',
  'arch/brother/brother.proto',
  'arch/c64/c64.proto',
  'arch/f85/f85.proto',
  'arch/fb100/fb100.proto',
  'arch/ibm/ibm.proto',
  'arch/macintosh/macintosh.proto',
  'arch/mx/mx.proto',
  'arch/victor9k/victor9k.proto',
  'arch/zilogmcz/zilogmcz.proto',
  'arch/tids990/tids990.proto',
  'arch/micropolis/micropolis.proto',
  'arch/northstar/northstar.proto',
  'arch/agat/agat.proto',
  preserve_path_from: meson.current_source_dir()
)

config_proto_lib = static_library(
    'config',
    sources: [
      config_proto_h_cc
    ]
)

config_proto_dep = declare_dependency(
    include_directories: [
      config_proto_lib.private_dir_include()
    ],
    link_with: [
      config_proto_lib
    ]
)

fl2_proto = protoc_generator.process('lib/fl2.proto',
  preserve_path_from: meson.current_source_dir()
)

libfluxengine_includes = include_directories('.', 'lib', 'arch')

libfluxengine_dep = declare_dependency(
    include_directories: [
      libfluxengine_includes,
    ],
    link_with: [
      static_library(
        'fluxengine',
        include_directories: [
          'arch',
          'lib',
        ],
        override_options: [ 'cpp_std=c++2a'],
        dependencies: [
          config_proto_dep,
          fmt_dep,
          libusbp_dep,
          stb_dep,
        ],
        sources: [
          fl2_proto,
          'arch/aeslanier/decoder.cc',
          'arch/agat/agat.cc',
          'arch/agat/decoder.cc',
          'arch/amiga/amiga.cc',
          'arch/amiga/decoder.cc',
          'arch/amiga/encoder.cc',
          'arch/apple2/decoder.cc',
          'arch/apple2/encoder.cc',
          'arch/brother/decoder.cc',
          'arch/brother/encoder.cc',
          'arch/c64/c64.cc',
          'arch/c64/decoder.cc',
          'arch/c64/encoder.cc',
          'arch/f85/decoder.cc',
          'arch/fb100/decoder.cc',
          'arch/ibm/decoder.cc',
          'arch/ibm/encoder.cc',
          'arch/macintosh/decoder.cc',
          'arch/macintosh/encoder.cc',
          'arch/micropolis/decoder.cc',
          'arch/micropolis/encoder.cc',
          'arch/mx/decoder.cc',
          'arch/northstar/decoder.cc',
          'arch/northstar/encoder.cc',
          'arch/tids990/decoder.cc',
          'arch/tids990/encoder.cc',
          'arch/victor9k/decoder.cc',
          'arch/victor9k/encoder.cc',
          'arch/zilogmcz/decoder.cc',
          'lib/bitmap.cc',
          'lib/bytes.cc',
          'lib/crc.cc',
          'lib/csvreader.cc',
          'lib/decoders/decoders.cc',
          'lib/decoders/fluxdecoder.cc',
          'lib/decoders/fluxmapreader.cc',
          'lib/decoders/fmmfm.cc',
          'lib/encoders/encoders.cc',
          'lib/flags.cc',
          'lib/fluxmap.cc',
          'lib/fluxsink/aufluxsink.cc',
          'lib/fluxsink/fl2fluxsink.cc',
          'lib/fluxsink/fluxsink.cc',
          'lib/fluxsink/hardwarefluxsink.cc',
          'lib/fluxsink/scpfluxsink.cc',
          'lib/fluxsink/vcdfluxsink.cc',
          'lib/fluxsource/cwffluxsource.cc',
          'lib/fluxsource/erasefluxsource.cc',
          'lib/fluxsource/fl2fluxsource.cc',
          'lib/fluxsource/fluxsource.cc',
          'lib/fluxsource/hardwarefluxsource.cc',
          'lib/fluxsource/kryoflux.cc',
          'lib/fluxsource/kryofluxfluxsource.cc',
          'lib/fluxsource/scpfluxsource.cc',
          'lib/fluxsource/testpatternfluxsource.cc',
          'lib/globals.cc',
          'lib/hexdump.cc',
          'lib/image.cc',
          'lib/imagereader/d64imagereader.cc',
          'lib/imagereader/d88imagereader.cc',
          'lib/imagereader/dimimagereader.cc',
          'lib/imagereader/diskcopyimagereader.cc',
          'lib/imagereader/fdiimagereader.cc',
          'lib/imagereader/imagereader.cc',
          'lib/imagereader/imdimagereader.cc',
          'lib/imagereader/imgimagereader.cc',
          'lib/imagereader/jv3imagereader.cc',
          'lib/imagereader/nfdimagereader.cc',
          'lib/imagereader/nsiimagereader.cc',
          'lib/imagereader/td0imagereader.cc',
          'lib/imagewriter/d64imagewriter.cc',
          'lib/imagewriter/d88imagewriter.cc',
          'lib/imagewriter/diskcopyimagewriter.cc',
          'lib/imagewriter/imagewriter.cc',
          'lib/imagewriter/imgimagewriter.cc',
          'lib/imagewriter/ldbsimagewriter.cc',
          'lib/imagewriter/nsiimagewriter.cc',
          'lib/imagewriter/rawimagewriter.cc',
          'lib/imginputoutpututils.cc',
          'lib/ldbs.cc',
          'lib/logger.cc',
          'lib/mapper.cc',
          'lib/proto.cc',
          'lib/readerwriter.cc',
          'lib/sector.cc',
          'lib/usb/fluxengineusb.cc',
          'lib/usb/greaseweazle.cc',
          'lib/usb/greaseweazleusb.cc',
          'lib/usb/serial.cc',
          'lib/usb/usb.cc',
          'lib/usb/usbfinder.cc',
          'lib/utils.cc',
        ]
      )
    ]
)

subdir('src')

