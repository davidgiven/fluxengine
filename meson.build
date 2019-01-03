project('fluxclient', 'c', 'cpp')

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
    ],
    include_directories: [fmtinc],
    link_with: [fmtlib],
    dependencies: [libusb]
)
feinc = include_directories('lib')

sqllib =     shared_library('sqllib',     ['lib/sql.cc'],                                     link_with: [felib], dependencies: [sqlite])
readerlib =  shared_library('readerlib',  ['lib/reader.cc'],   include_directories: [fmtinc], link_with: [felib, sqllib, fmtlib])

decoderlib = shared_library('decoderlib',
    [
        'lib/decoders/decoders.cc',
        'lib/decoders/mfmdecoder.cc',
        'lib/decoders/ibmparser.cc',
		'lib/brother/decoder.cc',
		'lib/brother/parser.cc',
    ],
    include_directories: [feinc, fmtinc],
    link_with: [felib, fmtlib]
)
decoderinc = include_directories('lib/decoders')

executable('fe-rpm',               ['src/fe-rpm.cc'],               include_directories: [feinc], link_with: [felib])
executable('fe-seek',              ['src/fe-seek.cc'],              include_directories: [feinc], link_with: [felib])
executable('fe-testbulktransport', ['src/fe-testbulktransport.cc'], include_directories: [feinc], link_with: [felib])
executable('fe-readibm',           ['src/fe-readibm.cc'],           include_directories: [feinc, fmtinc, decoderinc], link_with: [felib, readerlib, decoderlib, fmtlib])
executable('fe-readbrother',       ['src/fe-readbrother.cc'],       include_directories: [feinc, fmtinc, decoderinc], link_with: [felib, readerlib, decoderlib, fmtlib])
executable('fe-inspect',           ['src/fe-inspect.cc'],           include_directories: [feinc, fmtinc, decoderinc], link_with: [felib, readerlib, decoderlib, fmtlib])

