project('fluxclient', 'c', 'cpp')
add_global_arguments('--std=c++14', language: 'cpp')

libusb = dependency('libusb-1.0')
sqlite = dependency('sqlite3')
zlib = dependency('zlib')

fmtlib = declare_dependency(
    link_with:
        shared_library('fmtlib',
            [
                'dep/fmt/format.cc',
                'dep/fmt/posix.cc'
            ]
        ),
    include_directories:
        include_directories('dep/fmt')
)

crunchlib = declare_dependency(
    link_with:
        shared_library('crunch',
            ['lib/common/crunch.c']
        ),
    include_directories:
        include_directories('lib/common')
)

felib = declare_dependency(
    link_with:
        shared_library('felib',
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
            dependencies: [fmtlib, crunchlib, libusb, zlib]
        ),
    include_directories:
        include_directories('lib')
)

sqllib = declare_dependency(
    link_with:
        shared_library('sqllib',
            ['lib/sql.cc'],
            dependencies: [fmtlib, felib, sqlite, zlib]
        )
)

fluxreaderlib = declare_dependency(
    link_with:
        shared_library('fluxreaderlib',
            [
                'lib/fluxreader/fluxreader.cc',
                'lib/fluxreader/sqlitefluxreader.cc',
                'lib/fluxreader/hardwarefluxreader.cc',
                'lib/fluxreader/streamfluxreader.cc',
                'lib/fluxreader/kryoflux.cc',
            ],
            dependencies: [fmtlib, felib, sqllib, sqlite]
        ),
    include_directories:
        include_directories('lib/fluxreader')
)

fluxwriterlib = declare_dependency(
	link_with:
		shared_library('fluxwriterlib',
			[
				'lib/fluxwriter/fluxwriter.cc',
				'lib/fluxwriter/sqlitefluxwriter.cc',
				'lib/fluxwriter/hardwarefluxwriter.cc',
			],
			dependencies: [fmtlib, felib, sqllib, sqlite]
		),
	include_directories:
		include_directories('lib/fluxwriter')
)

decoderlib = declare_dependency(
    link_with:
        shared_library('decoderlib',
            [
                'lib/decoders/decoders.cc',
                'lib/decoders/fmmfm.cc',
            ],
            dependencies: [fmtlib, felib]
        ),
    include_directories:
        include_directories('lib/decoders')
)

readerlib = declare_dependency(
    link_with:
        shared_library('readerlib',
            ['lib/reader.cc'],
            dependencies: [fmtlib, felib, sqllib, decoderlib, fluxreaderlib, sqlite])
)
                        
writerlib = declare_dependency(
    link_with:
        shared_library('writerlib',
            ['lib/writer.cc'],
            dependencies: [fmtlib, felib, sqllib, fluxreaderlib, fluxwriterlib, sqlite]
        )
)

encoderlib = declare_dependency(
    link_with:
        shared_library('encoderlib',
            ['lib/encoder.cc'],
            dependencies: [fmtlib, felib]
        )
)

aeslanierdecoderlib = declare_dependency(
    link_with:
        shared_library('aeslanierdecoderlib',
        [ 'lib/aeslanier/decoder.cc', ],
        dependencies: [fmtlib, felib, decoderlib]),
    include_directories:
        include_directories('lib/aeslanier')
)

amigadecoderlib = declare_dependency(
    link_with:
        shared_library('amigadecoderlib',
        [ 'lib/amiga/decoder.cc', ],
        dependencies: [fmtlib, felib, decoderlib]),
    include_directories:
        include_directories('lib/amiga')
)

apple2decoderlib = declare_dependency(
    link_with:
        shared_library('apple2decoderlib',
        [ 'lib/apple2/decoder.cc', ],
        dependencies: [fmtlib, felib, decoderlib]),
    include_directories:
        include_directories('lib/apple2')
)

brotherdecoderlib = declare_dependency(
    link_with:
        shared_library('brotherdecoderlib',
        [ 'lib/brother/decoder.cc', ],
        dependencies: [fmtlib, felib, decoderlib, sqlite]),
    include_directories:
        include_directories('lib/brother')
)

brotherencoderlib = declare_dependency(
    link_with:
        shared_library('brotherencoderlib',
        [ 'lib/brother/encoder.cc', ],
        dependencies: [fmtlib, felib, decoderlib, encoderlib]),
    include_directories:
        include_directories('lib/brother')
)

c64decoderlib = declare_dependency(
    link_with:
        shared_library('c64decoderlib',
        [ 'lib/c64/decoder.cc', ],
        dependencies: [fmtlib, felib, decoderlib]),
    include_directories:
        include_directories('lib/c64')
)

f85decoderlib = declare_dependency(
    link_with:
        shared_library('f85decoderlib',
        [ 'lib/f85/decoder.cc', ],
        dependencies: [fmtlib, felib, decoderlib]),
    include_directories:
        include_directories('lib/f85')
)

ibmdecoderlib = declare_dependency(
    link_with:
        shared_library('ibmdecoderlib',
        [ 'lib/ibm/decoder.cc', ],
        dependencies: [fmtlib, felib, decoderlib]),
    include_directories:
        include_directories('lib/ibm')
)

macdecoderlib = declare_dependency(
    link_with:
        shared_library('macdecoderlib',
        [ 'lib/macintosh/decoder.cc', ],
        dependencies: [fmtlib, felib, decoderlib]),
    include_directories:
        include_directories('lib/macintosh')
)

zilogmczdecoderlib = declare_dependency(
    link_with:
        shared_library('zilogmczdecoderlib',
        [ 'lib/zilogmcz/decoder.cc', ],
        dependencies: [fmtlib, felib, decoderlib]),
    include_directories:
        include_directories('lib/zilogmcz')
)

victor9kdecoderlib = declare_dependency(
    link_with:
        shared_library('victor9kdecoderlib',
        [ 'lib/victor9k/decoder.cc', ],
        dependencies: [fmtlib, felib, decoderlib]),
    include_directories:
        include_directories('lib/victor9k')
)

executable('fe-erase',             ['src/fe-erase.cc'],             dependencies: [felib, writerlib])
executable('fe-inspect',           ['src/fe-inspect.cc'],           dependencies: [fmtlib, felib, decoderlib, readerlib])
executable('fe-readadfs',          ['src/fe-readadfs.cc'],          dependencies: [fmtlib, felib, decoderlib, readerlib, ibmdecoderlib])
executable('fe-readaeslanier',     ['src/fe-readaeslanier.cc'],     dependencies: [fmtlib, felib, decoderlib, readerlib, aeslanierdecoderlib])
executable('fe-readamiga',         ['src/fe-readamiga.cc'],         dependencies: [fmtlib, felib, decoderlib, readerlib, amigadecoderlib])
executable('fe-readampro',         ['src/fe-readampro.cc'],         dependencies: [fmtlib, felib, decoderlib, readerlib, ibmdecoderlib])
executable('fe-readapple2',        ['src/fe-readapple2.cc'],        dependencies: [fmtlib, felib, decoderlib, readerlib, apple2decoderlib])
executable('fe-readbrother',       ['src/fe-readbrother.cc'],       dependencies: [fmtlib, felib, decoderlib, readerlib, brotherdecoderlib])
executable('fe-readc64',           ['src/fe-readc64.cc'],           dependencies: [fmtlib, felib, decoderlib, readerlib, c64decoderlib])
executable('fe-readdfs',           ['src/fe-readdfs.cc'],           dependencies: [fmtlib, felib, decoderlib, readerlib, ibmdecoderlib])
executable('fe-readf85',           ['src/fe-readf85.cc'],           dependencies: [fmtlib, felib, decoderlib, readerlib, f85decoderlib])
executable('fe-readibm',           ['src/fe-readibm.cc'],           dependencies: [fmtlib, felib, decoderlib, readerlib, ibmdecoderlib])
executable('fe-readmac',           ['src/fe-readmac.cc'],           dependencies: [fmtlib, felib, decoderlib, readerlib, macdecoderlib])
executable('fe-readzilogmcz',      ['src/fe-readzilogmcz.cc'],      dependencies: [fmtlib, felib, decoderlib, readerlib, zilogmczdecoderlib])
executable('fe-readvictor9k',      ['src/fe-readvictor9k.cc'],      dependencies: [fmtlib, felib, decoderlib, readerlib, victor9kdecoderlib])
executable('fe-rpm',               ['src/fe-rpm.cc'],               dependencies: [fmtlib, felib])
executable('fe-seek',              ['src/fe-seek.cc'],              dependencies: [fmtlib, felib])
executable('fe-testbulktransport', ['src/fe-testbulktransport.cc'], dependencies: [fmtlib, felib])
executable('fe-upgradefluxfile',   ['src/fe-upgradefluxfile.cc'],   dependencies: [fmtlib, felib, sqllib, sqlite])
executable('fe-writebrother',      ['src/fe-writebrother.cc'],      dependencies: [fmtlib, felib, writerlib, decoderlib, encoderlib, brotherencoderlib])
executable('fe-writeflux',         ['src/fe-writeflux.cc'],         dependencies: [fmtlib, felib, readerlib, writerlib])
executable('fe-writetestpattern',  ['src/fe-writetestpattern.cc'],  dependencies: [fmtlib, felib, writerlib])

executable('brother120tool',       ['tools/brother120tool.cc'],     dependencies: [fmtlib, felib])
executable('cwftoflux',            ['tools/cwftoflux.cc'],          dependencies: [fmtlib, felib, sqllib, sqlite])

test('DataSpec',        executable('dataspec-test', ['tests/dataspec.cc'],             dependencies: [felib]))
test('Flags',           executable('flags-test', ['tests/flags.cc'],                   dependencies: [felib]))
test('FmMfm',           executable('fmmfm-test', ['tests/fmmfm.cc'],                   dependencies: [felib, decoderlib]))
test('BitAccumulator',  executable('bitaccumulator-test', ['tests/bitaccumulator.cc'], dependencies: [felib]))
test('Kryoflux',        executable('kryoflux-test', ['tests/kryoflux.cc'],             dependencies: [felib, decoderlib, fluxreaderlib]))
test('Compression',     executable('compression-test', ['tests/compression.cc'],       dependencies: [felib, decoderlib]))
test('Bytes',           executable('bytes-test', ['tests/bytes.cc'],                   dependencies: [felib]))
test('Crunch',          executable('crunch-test', ['tests/crunch.cc'],                 dependencies: [felib, crunchlib]))
