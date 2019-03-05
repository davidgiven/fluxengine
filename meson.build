project('fluxclient', 'cpp')
add_global_arguments('--std=c++14', language: 'cpp')

libusb = dependency('libusb-1.0')
sqlite = dependency('sqlite3')
zlib = dependency('zlib')

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
		'lib/bytes.cc',
    ],
    include_directories: [fmtinc],
    link_with: [fmtlib],
    dependencies: [libusb, zlib]
)
feinc = include_directories('lib')

sqllib = shared_library('sqllib',
	['lib/sql.cc'],
    include_directories: [feinc, fmtinc],
	link_with: [felib, fmtlib],
	dependencies: [sqlite]
)

streamlib = shared_library('streamlib',
    [ 'lib/stream/stream.cc', ],
    include_directories: [feinc, fmtinc],
    link_with: [felib, fmtlib]
)
streaminc = include_directories('lib/stream')

fluxreaderlib = shared_library('fluxreaderlib',
    [
        'lib/fluxreader/fluxreader.cc',
        'lib/fluxreader/sqlitefluxreader.cc',
        'lib/fluxreader/hardwarefluxreader.cc',
        'lib/fluxreader/streamfluxreader.cc',
    ],
    include_directories: [feinc, fmtinc, streaminc],
    link_with: [felib, streamlib, sqllib, fmtlib]
)
fluxreaderinc = include_directories('lib/fluxreader')

decoderlib = shared_library('decoderlib',
    [
        'lib/decoders/decoders.cc',
        'lib/decoders/segmenter.cc',
		'lib/decoders/fmmfm.cc',
    ],
    include_directories: [feinc, fmtinc],
    link_with: [felib, fmtlib]
)
decoderinc = include_directories('lib/decoders')

readerlib = shared_library('readerlib',
						['lib/reader.cc'],
						include_directories: [feinc, fmtinc, decoderinc, fluxreaderinc],
						link_with: [felib, sqllib, fmtlib, decoderlib, fluxreaderlib])
                        
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

aeslanierdecoderlib = shared_library('aeslanierdecoderlib',
    [
		'lib/aeslanier/decoder.cc',
    ],
    include_directories: [feinc, fmtinc, decoderinc],
    link_with: [felib, fmtlib, decoderlib]
)
aeslanierinc = include_directories('lib/aeslanier')

amigadecoderlib = shared_library('amigadecoderlib',
    [
		'lib/amiga/decoder.cc',
    ],
    include_directories: [feinc, fmtinc, decoderinc],
    link_with: [felib, fmtlib, decoderlib]
)
amigainc = include_directories('lib/amiga')

apple2decoderlib = shared_library('apple2decoderlib',
    [
		'lib/apple2/decoder.cc',
    ],
    include_directories: [feinc, fmtinc, decoderinc],
    link_with: [felib, fmtlib, decoderlib]
)
apple2inc = include_directories('lib/apple2')

brotherdecoderlib = shared_library('brotherdecoderlib',
    [
		'lib/brother/decoder.cc',
    ],
    include_directories: [feinc, fmtinc, decoderinc],
    link_with: [felib, fmtlib, decoderlib]
)
brotherencoderlib = shared_library('brotherencoderlib',
    [
		'lib/brother/encoder.cc',
    ],
    include_directories: [feinc, fmtinc, decoderinc],
    link_with: [felib, fmtlib, encoderlib, decoderlib]
)
brotherinc = include_directories('lib/brother')

c64decoderlib = shared_library('c64decoderlib',
    [
		'lib/c64/decoder.cc',
    ],
    include_directories: [feinc, fmtinc, decoderinc],
    link_with: [felib, fmtlib, decoderlib]
)
c64inc = include_directories('lib/c64')

ibmdecoderlib = shared_library('ibmdecoderlib',
    [
		'lib/ibm/decoder.cc',
    ],
    include_directories: [feinc, fmtinc, decoderinc],
    link_with: [felib, fmtlib, decoderlib]
)
ibminc = include_directories('lib/ibm')

macdecoderlib = shared_library('macdecoderlib',
    [
		'lib/macintosh/decoder.cc',
    ],
    include_directories: [feinc, fmtinc, decoderinc],
    link_with: [felib, fmtlib, decoderlib]
)
macinc = include_directories('lib/macintosh')

victor9kdecoderlib = shared_library('victor9kdecoderlib',
    [
		'lib/victor9k/decoder.cc',
    ],
    include_directories: [feinc, fmtinc, decoderinc],
    link_with: [felib, fmtlib, decoderlib]
)
victor9kinc = include_directories('lib/victor9k')

executable('fe-erase',             ['src/fe-erase.cc'],             include_directories: [feinc], link_with: [felib, writerlib])
executable('fe-inspect',           ['src/fe-inspect.cc'],           include_directories: [feinc, fmtinc, decoderinc], link_with: [felib, readerlib, decoderlib, fmtlib])
executable('fe-readadfs',          ['src/fe-readadfs.cc'],          include_directories: [feinc, fmtinc, decoderinc, ibminc], link_with: [felib, readerlib, decoderlib, ibmdecoderlib, fmtlib])
executable('fe-readaeslanier',     ['src/fe-readaeslanier.cc'],     include_directories: [feinc, fmtinc, decoderinc, aeslanierinc], link_with: [felib, readerlib, decoderlib, aeslanierdecoderlib, fmtlib])
executable('fe-readamiga',         ['src/fe-readamiga.cc'],         include_directories: [feinc, fmtinc, decoderinc, amigainc], link_with: [felib, readerlib, decoderlib, amigadecoderlib, fmtlib])
executable('fe-readampro',         ['src/fe-readampro.cc'],         include_directories: [feinc, fmtinc, decoderinc, ibminc], link_with: [felib, readerlib, decoderlib, ibmdecoderlib, fmtlib])
executable('fe-readapple2',        ['src/fe-readapple2.cc'],        include_directories: [feinc, fmtinc, decoderinc, apple2inc], link_with: [felib, readerlib, decoderlib, apple2decoderlib, fmtlib])
executable('fe-readbrother',       ['src/fe-readbrother.cc'],       include_directories: [feinc, fmtinc, decoderinc, brotherinc], link_with: [felib, readerlib, decoderlib, brotherdecoderlib, fmtlib])
executable('fe-readc64',           ['src/fe-readc64.cc'],           include_directories: [feinc, fmtinc, decoderinc, c64inc], link_with: [felib, readerlib, decoderlib, c64decoderlib, fmtlib])
executable('fe-readdfs',           ['src/fe-readdfs.cc'],           include_directories: [feinc, fmtinc, decoderinc, ibminc], link_with: [felib, readerlib, decoderlib, ibmdecoderlib, fmtlib])
executable('fe-readibm',           ['src/fe-readibm.cc'],           include_directories: [feinc, fmtinc, decoderinc, ibminc], link_with: [felib, readerlib, decoderlib, ibmdecoderlib, fmtlib])
executable('fe-readmac',           ['src/fe-readmac.cc'],           include_directories: [feinc, fmtinc, decoderinc, macinc], link_with: [felib, readerlib, decoderlib, macdecoderlib, fmtlib])
executable('fe-readvictor9k',      ['src/fe-readvictor9k.cc'],      include_directories: [feinc, fmtinc, decoderinc, victor9kinc], link_with: [felib, readerlib, decoderlib, victor9kdecoderlib, fmtlib])
executable('fe-rpm',               ['src/fe-rpm.cc'],               include_directories: [feinc], link_with: [felib])
executable('fe-seek',              ['src/fe-seek.cc'],              include_directories: [feinc], link_with: [felib])
executable('fe-testbulktransport', ['src/fe-testbulktransport.cc'], include_directories: [feinc], link_with: [felib])
executable('fe-upgradefluxfile',   ['src/fe-upgradefluxfile.cc'],   include_directories: [feinc, fmtinc], link_with: [felib, fmtlib, sqllib])
executable('fe-writebrother',      ['src/fe-writebrother.cc'],      include_directories: [feinc, fmtinc, decoderinc, brotherinc], link_with: [felib, writerlib, decoderlib, encoderlib, brotherencoderlib, fmtlib])
executable('fe-writeflux',         ['src/fe-writeflux.cc'],         include_directories: [feinc, fmtinc], link_with: [felib, readerlib, writerlib, fmtlib])
executable('fe-writetestpattern',  ['src/fe-writetestpattern.cc'],  include_directories: [feinc, fmtinc], link_with: [felib, writerlib, fmtlib])

executable('brother120tool',       ['tools/brother120tool.cc'],     include_directories: [feinc, fmtinc], link_with: [felib, fmtlib])
executable('cwftoflux',            ['tools/cwftoflux.cc'],          include_directories: [feinc, fmtinc], link_with: [felib, fmtlib, sqllib])

test('DataSpec',        executable('dataspec-test', ['tests/dataspec.cc'],             include_directories: [feinc], link_with: [felib]))
test('Flags',           executable('flags-test', ['tests/flags.cc'],                   include_directories: [feinc], link_with: [felib]))
test('FmMfm',           executable('fmmfm-test', ['tests/fmmfm.cc'],                   include_directories: [feinc, decoderinc], link_with: [felib, decoderlib]))
test('BitAccumulator',  executable('bitaccumulator-test', ['tests/bitaccumulator.cc'], include_directories: [feinc], link_with: [felib]))
test('Kryoflux',        executable('kryoflux-test', ['tests/kryoflux.cc'],             include_directories: [feinc, decoderinc, streaminc], link_with: [felib, decoderlib, streamlib]))
test('Compression',     executable('compression-test', ['tests/compression.cc'],       include_directories: [feinc, decoderinc, streaminc], link_with: [felib, decoderlib, streamlib]))
test('Bytes',           executable('bytes-test', ['tests/bytes.cc'],                   include_directories: [feinc], link_with: [felib]))
