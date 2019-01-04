project('fluxclient', 'cpp')
add_global_arguments('--std=c++14', language: 'cpp')
add_global_arguments('-Og', language: 'cpp')

libusb = dependency('libusb-1.0')
sqlite = dependency('sqlite3')

fmtlib = shared_library('fmtlib',
    [
        'dep/fmt/format.cc',
        'dep/fmt/posix.cc'
    ])
fmtinc = include_directories('dep/fmt')

felib = shared_library('felib',
    [
        'lib/flags.cc',
        'lib/fluxmap.cc',
        'lib/globals.cc',
        'lib/usb.cc',
        'lib/image.cc',
		'lib/crc.cc',
		'lib/hexdump.cc',
		'lib/sectorset.cc',
    ],
    include_directories: [fmtinc],
    link_with: [fmtlib],
    dependencies: [libusb]
)
feinc = include_directories('lib')

sqllib = shared_library('sqllib',
						['lib/sql.cc'],
						link_with: [felib],
						dependencies: [sqlite])
readerlib = shared_library('readerlib',
						['lib/reader.cc'],
						include_directories: [fmtinc],
						link_with: [felib, sqllib, fmtlib])
writerlib =  shared_library('writerlib',
						['lib/writer.cc'],
						include_directories: [fmtinc],
						link_with: [felib, sqllib, fmtlib])

decoderlib = shared_library('decoderlib',
    [
        'lib/decoders/decoders.cc',
        'lib/decoders/mfmdecoder.cc',
        'lib/decoders/ibmparser.cc',
    ],
    include_directories: [feinc, fmtinc],
    link_with: [felib, fmtlib]
)
decoderinc = include_directories('lib/decoders')

encoderlib = shared_library('encoderlib',
    [
		'lib/encoder.cc',
    ],
    include_directories: [feinc, fmtinc],
    link_with: [felib, fmtlib]
)

brotherdecoderlib = shared_library('brotherdecoderlib',
    [
		'lib/brother/decoder.cc',
		'lib/brother/parser.cc',
    ],
    include_directories: [feinc, fmtinc, decoderinc],
    link_with: [felib, fmtlib, decoderlib]
)
brotherencoderlib = shared_library('brotherencoderlib',
    [
		'lib/brother/encoder.cc',
    ],
    include_directories: [feinc, fmtinc],
    link_with: [felib, fmtlib, encoderlib]
)
brotherinc = include_directories('lib/brother')

executable('fe-rpm',               ['src/fe-rpm.cc'],               include_directories: [feinc], link_with: [felib])
executable('fe-seek',              ['src/fe-seek.cc'],              include_directories: [feinc], link_with: [felib])
executable('fe-testbulktransport', ['src/fe-testbulktransport.cc'], include_directories: [feinc], link_with: [felib])
executable('fe-readibm',           ['src/fe-readibm.cc'],           include_directories: [feinc, fmtinc, decoderinc], link_with: [felib, readerlib, decoderlib, fmtlib])
executable('fe-readbrother',       ['src/fe-readbrother.cc'],       include_directories: [feinc, fmtinc, decoderinc, brotherinc], link_with: [felib, readerlib, decoderlib, brotherdecoderlib, fmtlib])
executable('fe-writebrother',      ['src/fe-writebrother.cc'],      include_directories: [feinc, fmtinc, brotherinc], link_with: [felib, writerlib, encoderlib, brotherencoderlib, fmtlib])
executable('fe-inspect',           ['src/fe-inspect.cc'],           include_directories: [feinc, fmtinc, decoderinc], link_with: [felib, readerlib, decoderlib, fmtlib])

