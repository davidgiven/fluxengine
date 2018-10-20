project('fluxclient', 'c', 'cpp')

libusb = dependency('libusb-1.0')
sqlite = dependency('sqlite3')

executable('fluxclient',
    [
        'oldclient/main.c',
        'oldclient/usb.c',
        'oldclient/sql.c',
        'oldclient/encoder.c',
        'oldclient/fluxmap.c',
        'oldclient/cmd_rpm.c',
        'oldclient/cmd_usbbench.c',
        'oldclient/cmd_read.c',
        'oldclient/cmd_write.c',
        'oldclient/cmd_mfmdecode.c',
        'oldclient/cmd_brotherdecode.c',
        'oldclient/cmd_testpattern.c',
        'oldclient/cmd_fluxdump.c',
        'oldclient/cmd_calibrate.c',
        'oldclient/cmd_getclock.c',
    ],
    dependencies: [libusb, sqlite]
)

felib = shared_library('felib',
    [
        'lib/flags.cc',
        'lib/fluxmap.cc',
        'lib/globals.cc',
        'lib/usb.cc',
    ],
    dependencies: [libusb]
)

sqllib = shared_library('sqllib', ['lib/sql.cc'], link_with: [felib], dependencies: [sqlite])
readerlib = shared_library('readerlib', ['lib/reader.cc'], link_with: [felib, sqllib])

feinc = include_directories('lib')

executable('fe-rpm',               ['src/fe-rpm.cc'],               include_directories: [feinc], link_with: [felib])
executable('fe-seek',              ['src/fe-seek.cc'],              include_directories: [feinc], link_with: [felib])
executable('fe-testbulktransport', ['src/fe-testbulktransport.cc'], include_directories: [feinc], link_with: [felib])
executable('fe-readibm',           ['src/fe-readibm.cc'],           include_directories: [feinc], link_with: [felib, readerlib])

