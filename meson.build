project('fluxclient', 'cpp')
add_global_arguments('--std=c++14', language: 'cpp')

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
		'lib/crc.cc',
        'lib/dataspec.cc',
		'lib/hexdump.cc',
		'lib/sectorset.cc',
        'lib/flags.cc',
        'lib/fluxmap.cc',
        'lib/globals.cc',
        'lib/image.cc',
        'lib/sector.cc',
        'lib/usb.cc',
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

decoderlib = shared_library('decoderlib',
    [
        'lib/decoders/decoders.cc',
        'lib/decoders/fmdecoder.cc',
        'lib/decoders/mfmdecoder.cc',
        'lib/decoders/ibmparser.cc',
    ],
    include_directories: [feinc, fmtinc],
    link_with: [felib, fmtlib]
)
decoderinc = include_directories('lib/decoders')

readerlib = shared_library('readerlib',
						['lib/reader.cc'],
						include_directories: [fmtinc, decoderinc],
						link_with: [felib, sqllib, fmtlib, decoderlib])
                        
writerlib =  shared_library('writerlib',
						['lib/writer.cc'],
						include_directories: [fmtinc],
						link_with: [felib, sqllib, fmtlib])

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

executable('fe-erase',             ['src/fe-erase.cc'],             include_directories: [feinc], link_with: [felib, writerlib])
executable('fe-inspect',           ['src/fe-inspect.cc'],           include_directories: [feinc, fmtinc, decoderinc], link_with: [felib, readerlib, decoderlib, fmtlib])
executable('fe-readadfs',          ['src/fe-readadfs.cc'],          include_directories: [feinc, fmtinc, decoderinc], link_with: [felib, readerlib, decoderlib, fmtlib])
executable('fe-readbrother',       ['src/fe-readbrother.cc'],       include_directories: [feinc, fmtinc, decoderinc, brotherinc], link_with: [felib, readerlib, decoderlib, brotherdecoderlib, fmtlib])
executable('fe-readdfs',           ['src/fe-readdfs.cc'],           include_directories: [feinc, fmtinc, decoderinc], link_with: [felib, readerlib, decoderlib, fmtlib])
executable('fe-readibm',           ['src/fe-readibm.cc'],           include_directories: [feinc, fmtinc, decoderinc], link_with: [felib, readerlib, decoderlib, fmtlib])
executable('fe-rpm',               ['src/fe-rpm.cc'],               include_directories: [feinc], link_with: [felib])
executable('fe-seek',              ['src/fe-seek.cc'],              include_directories: [feinc], link_with: [felib])
executable('fe-testbulktransport', ['src/fe-testbulktransport.cc'], include_directories: [feinc], link_with: [felib])
executable('fe-writebrother',      ['src/fe-writebrother.cc'],      include_directories: [feinc, fmtinc, brotherinc], link_with: [felib, writerlib, encoderlib, brotherencoderlib, fmtlib])
executable('fe-writeflux',         ['src/fe-writeflux.cc'],         include_directories: [feinc, fmtinc], link_with: [felib, readerlib, writerlib, fmtlib])
executable('fe-writetestpattern',  ['src/fe-writetestpattern.cc'],  include_directories: [feinc, fmtinc], link_with: [felib, writerlib, fmtlib])

test('DataSpec', executable('dataspec-test', ['tests/dataspec.cc'], include_directories: [feinc], link_with: [felib]))
test('Flags',    executable('flags-test', ['tests/flags.cc'], include_directories: [feinc], link_with: [felib]))
