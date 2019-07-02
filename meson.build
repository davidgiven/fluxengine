project('fluxclient', 'c', 'cpp')
add_global_arguments('--std=c++14', language: 'cpp')

libusb = dependency('libusb-1.0')
sqlite = dependency('sqlite3')
zlib = dependency('zlib')

fmtlib = declare_dependency(
    link_with:
        static_library('fmtlib',
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
        static_library('crunch',
            ['lib/common/crunch.c']
        ),
    include_directories:
        include_directories('lib/common')
)

felib = declare_dependency(
    link_with:
        static_library('felib',
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
        static_library('sqllib',
            ['lib/sql.cc'],
            dependencies: [fmtlib, felib, sqlite, zlib]
        )
)

fluxsourcelib = declare_dependency(
    link_with:
        static_library('fluxsourcelib',
            [
                'lib/fluxsource/fluxsource.cc',
                'lib/fluxsource/sqlitefluxsource.cc',
                'lib/fluxsource/hardwarefluxsource.cc',
                'lib/fluxsource/streamfluxsource.cc',
                'lib/fluxsource/kryoflux.cc',
            ],
            dependencies: [fmtlib, felib, sqllib, sqlite]
        ),
    include_directories:
        include_directories('lib/fluxsource')
)

fluxsinklib = declare_dependency(
	link_with:
		static_library('fluxsinklib',
			[
				'lib/fluxsink/fluxsink.cc',
				'lib/fluxsink/sqlitefluxsink.cc',
				'lib/fluxsink/hardwarefluxsink.cc',
			],
			dependencies: [fmtlib, felib, sqllib, sqlite]
		),
	include_directories:
		include_directories('lib/fluxsink')
)

decoderlib = declare_dependency(
    link_with:
        static_library('decoderlib',
            [
                'lib/decoders/decoders.cc',
                'lib/decoders/fmmfm.cc',
                'lib/decoders/fluxmapreader.cc',
            ],
            dependencies: [fmtlib, felib]
        ),
    include_directories:
        include_directories('lib/decoders')
)

readerlib = declare_dependency(
    link_with:
        static_library('readerlib',
            [
                'lib/reader.cc'],
            dependencies: [fmtlib, felib, sqllib, decoderlib, fluxsourcelib, sqlite])
)
                        
writerlib = declare_dependency(
    link_with:
        static_library('writerlib',
            ['lib/writer.cc'],
            dependencies: [fmtlib, felib, sqllib, fluxsourcelib, fluxsinklib, sqlite]
        )
)

encoderlib = declare_dependency(
    link_with:
        static_library('encoderlib',
            ['lib/encoder.cc'],
            dependencies: [fmtlib, felib]
        )
)

aeslanierdecoderlib = declare_dependency(
   link_with:
       static_library('aeslanierdecoderlib',
       [ 'lib/aeslanier/decoder.cc', ],
       dependencies: [fmtlib, felib, decoderlib]),
   include_directories:
       include_directories('lib/aeslanier')
)

amigadecoderlib = declare_dependency(
   link_with:
       static_library('amigadecoderlib',
       [ 'lib/amiga/decoder.cc', ],
       dependencies: [fmtlib, felib, decoderlib]),
   include_directories:
       include_directories('lib/amiga')
)

apple2decoderlib = declare_dependency(
   link_with:
       static_library('apple2decoderlib',
       [ 'lib/apple2/decoder.cc', ],
       dependencies: [fmtlib, felib, decoderlib]),
   include_directories:
       include_directories('lib/apple2')
)

brotherdecoderlib = declare_dependency(
   link_with:
       static_library('brotherdecoderlib',
       [ 'lib/brother/decoder.cc', ],
       dependencies: [fmtlib, felib, decoderlib, sqlite]),
   include_directories:
       include_directories('lib/brother')
)

brotherencoderlib = declare_dependency(
   link_with:
       static_library('brotherencoderlib',
       [ 'lib/brother/encoder.cc', ],
       dependencies: [fmtlib, felib, decoderlib, encoderlib]),
   include_directories:
       include_directories('lib/brother')
)

c64decoderlib = declare_dependency(
   link_with:
       static_library('c64decoderlib',
       [ 'lib/c64/decoder.cc', ],
       dependencies: [fmtlib, felib, decoderlib]),
   include_directories:
       include_directories('lib/c64')
)

f85decoderlib = declare_dependency(
    link_with:
        static_library('f85decoderlib',
        [ 'lib/f85/decoder.cc', ],
        dependencies: [fmtlib, felib, decoderlib]),
    include_directories:
        include_directories('lib/f85')
)

fb100decoderlib = declare_dependency(
    link_with:
        static_library('fb100decoderlib',
        [ 'lib/fb100/decoder.cc', ],
        dependencies: [fmtlib, felib, decoderlib]),
    include_directories:
        include_directories('lib/fb100')
)

ibmdecoderlib = declare_dependency(
    link_with:
        static_library('ibmdecoderlib',
        [ 'lib/ibm/decoder.cc', ],
        dependencies: [fmtlib, felib, decoderlib]),
    include_directories:
        include_directories('lib/ibm')
)

macdecoderlib = declare_dependency(
    link_with:
        static_library('macdecoderlib',
        [ 'lib/macintosh/decoder.cc', ],
        dependencies: [fmtlib, felib, decoderlib]),
    include_directories:
        include_directories('lib/macintosh')
)

zilogmczdecoderlib = declare_dependency(
    link_with:
        static_library('zilogmczdecoderlib',
        [ 'lib/zilogmcz/decoder.cc', ],
        dependencies: [fmtlib, felib, decoderlib]),
    include_directories:
        include_directories('lib/zilogmcz')
)

victor9kdecoderlib = declare_dependency(
    link_with:
        static_library('victor9kdecoderlib',
        [ 'lib/victor9k/decoder.cc', ],
        dependencies: [fmtlib, felib, decoderlib]),
    include_directories:
        include_directories('lib/victor9k')
)

executable('fluxengine',
    [
        'src/fluxengine.cc',
        'src/fe-erase.cc',
        'src/fe-inspect.cc',
        'src/fe-readadfs.cc',
        'src/fe-readaeslanier.cc',
        'src/fe-readamiga.cc',
        'src/fe-readampro.cc',
        'src/fe-readapple2.cc',
        'src/fe-readbrother.cc',
        'src/fe-readc64.cc',
        'src/fe-readdfs.cc',
        'src/fe-readf85.cc',
        'src/fe-readfb100.cc',
        'src/fe-readibm.cc',
        'src/fe-readmac.cc',
        'src/fe-readzilogmcz.cc',
        'src/fe-readvictor9k.cc',
        'src/fe-rpm.cc',
        'src/fe-seek.cc',
        'src/fe-testbulktransport.cc',
        'src/fe-upgradefluxfile.cc',
        'src/fe-writebrother.cc',
        'src/fe-writeflux.cc',
        'src/fe-writetestpattern.cc',
    ],
    dependencies: [
        aeslanierdecoderlib,
        amigadecoderlib,
        apple2decoderlib,
        brotherdecoderlib,
        brotherencoderlib,
        c64decoderlib,
        decoderlib,
        encoderlib,
        f85decoderlib,
        fb100decoderlib,
        felib,
        fmtlib,
        ibmdecoderlib,
        macdecoderlib,
        readerlib,
        sqllib,
        victor9kdecoderlib,
        writerlib,
        zilogmczdecoderlib,
    ]
)

executable('brother120tool',       ['tools/brother120tool.cc'],     dependencies: [fmtlib, felib])
executable('cwftoflux',            ['tools/cwftoflux.cc'],          dependencies: [fmtlib, felib, sqllib, sqlite])

test('DataSpec',        executable('dataspec-test', ['tests/dataspec.cc'],             dependencies: [felib]))
test('Flags',           executable('flags-test', ['tests/flags.cc'],                   dependencies: [felib]))
test('FmMfm',           executable('fmmfm-test', ['tests/fmmfm.cc'],                   dependencies: [felib, decoderlib]))
test('BitAccumulator',  executable('bitaccumulator-test', ['tests/bitaccumulator.cc'], dependencies: [felib]))
test('Kryoflux',        executable('kryoflux-test', ['tests/kryoflux.cc'],             dependencies: [felib, decoderlib, fluxsourcelib]))
test('Compression',     executable('compression-test', ['tests/compression.cc'],       dependencies: [felib, decoderlib]))
test('Bytes',           executable('bytes-test', ['tests/bytes.cc'],                   dependencies: [felib]))
test('Crunch',          executable('crunch-test', ['tests/crunch.cc'],                 dependencies: [felib, crunchlib]))
test('FluxPattern',     executable('fluxpattern-test', ['tests/fluxpattern.cc'],       dependencies: [felib, decoderlib]))
