#!/bin/sh
set -e

cat <<EOF
rule cxx
    command = $CXX $CFLAGS \$flags -I. -c -o \$out \$in -MMD -MF \$out.d
    description = CXX \$in
    depfile = \$out.d
    deps = gcc
    
rule library
    command = $AR \$out \$in
    description = AR \$in

rule link
    command = $CXX $LDFLAGS -o \$out \$in \$flags $LIBS
    description = LINK \$in

rule test
    command = \$in && touch \$out
    description = TEST \$in

rule strip
    command = cp -f \$in \$out && $STRIP \$out
    description = STRIP \$in
EOF

buildlibrary() {
    local lib
    lib=$1
    shift

    local flags
    flags=
    while true; do
        case $1 in
            -*)
                flags="$flags $1"
                shift
                ;;

            *)
                break
        esac
    done

    local objs
    objs=
    for src in "$@"; do
        local obj
        obj="$OBJDIR/${src%%.c*}.o"
        objs="$objs $obj"

        echo build $obj : cxx $src
        echo "    flags=$flags"
    done

    echo build $OBJDIR/$lib : library $objs
}

buildprogram() {
    local prog
    prog=$1
    shift

    local flags
    flags=
    while true; do
        case $1 in
            -*)
                flags="$flags $1"
                shift
                ;;

            *)
                break
        esac
    done

    local objs
    objs=
    for src in "$@"; do
        objs="$objs $OBJDIR/$src"
    done

    echo build $prog-debug$EXTENSION : link $objs
    echo "    flags=$flags"

    echo build $prog$EXTENSION : strip $prog-debug$EXTENSION
}

buildsimpleprogram() {
	local prog
	prog=$1
	shift

    local flags
    flags=
    while true; do
        case $1 in
            -*)
                flags="$flags $1"
                shift
                ;;

            *)
                break
        esac
    done

	local src
	src=$1
	shift

	buildlibrary lib$prog.a $flags $src
	buildprogram $prog lib$prog.a "$@"
}

runtest() {
    local prog
    prog=$1
    shift

    buildlibrary lib$prog.a \
        "$@"

    buildprogram $OBJDIR/$prog \
        lib$prog.a \
        libbackend.a \
        libfmt.a

    echo build $OBJDIR/$prog.stamp : test $OBJDIR/$prog-debug$EXTENSION
}

buildlibrary libfmt.a \
    dep/fmt/format.cc \
    dep/fmt/posix.cc \

buildlibrary libbackend.a \
	lib/imagereader/imagereader.cc \
	lib/imagereader/imgimagereader.cc \
	lib/imagewriter/d64imagewriter.cc \
	lib/imagewriter/imagewriter.cc \
	lib/imagewriter/imgimagewriter.cc \
	lib/imagewriter/ldbsimagewriter.cc \
    arch/aeslanier/decoder.cc \
    arch/amiga/decoder.cc \
    arch/apple2/decoder.cc \
    arch/brother/decoder.cc \
    arch/brother/encoder.cc \
    arch/c64/decoder.cc \
    arch/f85/decoder.cc \
    arch/fb100/decoder.cc \
    arch/ibm/decoder.cc \
    arch/macintosh/decoder.cc \
    arch/mx/decoder.cc \
    arch/victor9k/decoder.cc \
    arch/zilogmcz/decoder.cc \
    lib/bytes.cc \
    lib/common/crunch.c \
    lib/crc.cc \
    lib/dataspec.cc \
    lib/decoders/decoders.cc \
    lib/decoders/fluxmapreader.cc \
    lib/decoders/fmmfm.cc \
    lib/encoders/encoders.cc \
    lib/flags.cc \
    lib/fluxmap.cc \
    lib/fluxsink/fluxsink.cc \
    lib/fluxsink/hardwarefluxsink.cc \
    lib/fluxsink/sqlitefluxsink.cc \
    lib/fluxsource/fluxsource.cc \
    lib/fluxsource/hardwarefluxsource.cc \
    lib/fluxsource/quickdiskfluxsource.cc \
    lib/fluxsource/kryoflux.cc \
    lib/fluxsource/sqlitefluxsource.cc \
    lib/fluxsource/streamfluxsource.cc \
    lib/globals.cc \
    lib/hexdump.cc \
    lib/image.cc \
    lib/ldbs.cc \
    lib/reader.cc \
    lib/sector.cc \
    lib/sectorset.cc \
    lib/sql.cc \
    lib/usb.cc \
    lib/writer.cc \

buildlibrary libfrontend.a \
    src/fe-cwftoflux.cc \
    src/fe-erase.cc \
    src/fe-fluxtoau.cc \
    src/fe-fluxtovcd.cc \
    src/fe-inspect.cc \
    src/fe-readadfs.cc \
    src/fe-readaeslanier.cc \
    src/fe-readamiga.cc \
    src/fe-readampro.cc \
    src/fe-readapple2.cc \
    src/fe-readbrother.cc \
    src/fe-readc64.cc \
    src/fe-readdfs.cc \
    src/fe-readf85.cc \
    src/fe-readfb100.cc \
    src/fe-readibm.cc \
    src/fe-readmac.cc \
    src/fe-readmx.cc \
    src/fe-readqd.cc \
    src/fe-readvictor9k.cc \
    src/fe-readzilogmcz.cc \
    src/fe-rpm.cc \
    src/fe-seek.cc \
    src/fe-testbulktransport.cc \
    src/fe-upgradefluxfile.cc \
    src/fe-writebrother.cc \
    src/fe-writeflux.cc \
    src/fe-writetestpattern.cc \
    src/fluxengine.cc \

buildprogram fluxengine \
    libfrontend.a \
    libbackend.a \
    libfmt.a \

buildlibrary libemu.a \
    dep/emu/fnmatch.c

buildsimpleprogram brother120tool \
	-Idep/emu \
    tools/brother120tool.cc \
    libbackend.a \
    libemu.a \
    libfmt.a \

runtest bitaccumulator-test tests/bitaccumulator.cc
runtest bytes-test          tests/bytes.cc
runtest compression-test    tests/compression.cc
runtest crunch-test         tests/crunch.cc
runtest dataspec-test       tests/dataspec.cc
runtest flags-test          tests/flags.cc
runtest fluxpattern-test    tests/fluxpattern.cc
runtest fmmfm-test          tests/fmmfm.cc
runtest kryoflux-test       tests/kryoflux.cc
runtest ldbs-test           tests/ldbs.cc
