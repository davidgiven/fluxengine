#!/bin/sh
set -e

cat <<EOF
rule cxx
    command = $CXX $CXXFLAGS \$flags -I. -c -o \$out \$in -MMD -MF \$out.d
    description = CXX \$out
    depfile = \$out.d
    deps = gcc
    
rule cc
    command = $CC $CFLAGS \$flags -I. -c -o \$out \$in -MMD -MF \$out.d
    description = CC \$out
    depfile = \$out.d
    deps = gcc
    
rule cobjc
    command = $COBJC $CFLAGS \$flags -I. -c -o \$out \$in -MMD -MF \$out.d
    description = COBJC \$out
    depfile = \$out.d
    deps = gcc
    
rule proto
    command = (echo \$in > \$def) && $PROTOC \$flags \$in
    description = PROTO \$in
    restat = yes

rule protoencode
    command = (echo '#include <string>' && echo 'static const unsigned char data[] = {' && $PROTOC \$flags --encode=\$messagetype \$\$(cat \$def)< \$in > \$out.bin && $XXD -i < \$out.bin && echo '}; extern std::string \$name(); std::string \$name() { return std::string((const char*)data, sizeof(data)); }') > \$out
    description = PROTOENCODE \$in
    restat = yes

rule binencode
    command = xxd -i \$in > \$out
    description = XXD \$in
    restat = true

rule library
    command = $AR \$out \$in && $RANLIB \$out
    description = AR \$in

rule link
    command = $CXX $LDFLAGS -o \$out \$in \$flags $LIBS
    description = LINK \$in

rule linkgui
    command = $CXX $LDFLAGS $GUILDFLAGS -o \$out \$in \$flags $LIBS $GUILIBS
    description = LINK-GUI \$in

rule test
    command = \$in && touch \$out
    description = TEST \$in

rule encodedecode
    command = sh scripts/encodedecodetest.sh \$format \$fluxx \$configs > \$out
    description = ENCODEDECODE \$fluxx \$format

rule strip
    command = cp -f \$in \$out && $STRIP \$out
    description = STRIP \$in

rule mktable
    command = sh scripts/mktable.sh \$kind \$words > \$out
    description = MKTABLE \$kind
    restat = true
EOF

buildlibrary() {
    local lib
    lib=$1
    shift

    local flags
    flags=
    local deps
    deps=
    while true; do
        case $1 in
            -d)
                deps="$deps $2"
                shift
                shift
                ;;

            -*)
                flags="$flags $1"
                shift
                ;;

            *)
                break
        esac
    done

    local oobjs
    local dobjs
    oobjs=
    dobjs=
    for src in "$@"; do
        local obj
        local dobj
        obj="$OBJDIR/opt/${src%%.c*}.o"
        oobjs="$oobjs $obj"
        dobj="$OBJDIR/dbg/${src%%.c*}.o"
        dobjs="$dobjs $dobj"


        case "${src##*.}" in
            m)
                echo "build $obj : cobjc $src | $deps"
                echo "    flags=$flags $COPTFLAGS"
                echo "build $dobj : cobjc $src | $deps"
                echo "    flags=$flags $CDBGFLAGS"
                ;;

            c)
                echo "build $obj : cc $src | $deps"
                echo "    flags=$flags $COPTFLAGS"
                echo "build $dobj : cc $src | $deps"
                echo "    flags=$flags $CDBGFLAGS"
                ;;

            cc|cpp)
                echo "build $obj : cxx $src | $deps"
                echo "    flags=$flags $COPTFLAGS"
                echo "build $dobj : cxx $src | $deps"
                echo "    flags=$flags $CDBGFLAGS"
                ;;

            *)
                echo "Unknown file extension" >&2
                exit 1
                ;;
        esac

    done

    echo build $OBJDIR/opt/$lib : library $oobjs
    echo build $OBJDIR/dbg/$lib : library $dobjs
}

buildproto() {
    local lib
    lib=$1
    shift

    local def
    def=$OBJDIR/proto/${lib%%.a}.def

    local flags
    flags=
    local deps
    deps=
    while true; do
        case $1 in
            -d)
                deps="$deps $2"
                shift
                shift
                ;;

            -*)
                flags="$flags $1"
                shift
                ;;

            *)
                break
        esac
    done

    local cfiles
    local hfiles
    cfiles=
    hfiles=
    for src in "$@"; do
        local cfile
        local hfile
        cfile="$OBJDIR/proto/${src%%.proto}.pb.cc"
        hfile="$OBJDIR/proto/${src%%.proto}.pb.h"
        cfiles="$cfiles $cfile"
        hfiles="$hfiles $hfile"
    done

    echo "build $cfiles $hfiles $def : proto $@ | $deps"
    echo "    flags=$flags --cpp_out=$OBJDIR/proto"
    echo "    def=$def"

    buildlibrary $lib -I$OBJDIR/proto $cfiles
}

buildencodedproto() {
    local def
    local message
    local name
    local input
    local output
    def=$1
    message=$2
    name=$3
    input=$4
    output=$5

    echo "build $output : protoencode $input | $def"
    echo "    name=$name"
    echo "    def=$def"
    echo "    messagetype=$message"
}

buildprogram() {
    local prog
    prog=$1
    shift

    local flags
    flags=
    local rule
    rule=link
    while true; do
        case $1 in
            -rule)
                rule=$2
                shift
                shift
                ;;

            -*)
                flags="$flags $1"
                shift
                ;;

            *)
                break
        esac
    done

    local oobjs
    local dobjs
    oobjs=
    dobjs=
    for src in "$@"; do
        oobjs="$oobjs $OBJDIR/opt/$src"
        dobjs="$dobjs $OBJDIR/dbg/$src"
    done

    echo build $prog-debug$EXTENSION : $rule $dobjs
    echo "    flags=$flags $LDDBGFLAGS"

    echo build $prog$EXTENSION-unstripped : $rule $oobjs
    echo "    flags=$flags $LDOPTFLAGS"

    echo build $prog$EXTENSION : strip $prog$EXTENSION-unstripped
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

buildmktable() {
    local kind
    local out
    kind=$1
    out=$2
    shift
    shift

    echo "build $out : mktable scripts/mktable.sh"
    echo "    words=$@"
    echo "    kind=$kind"
}

runtest() {
    local prog
    prog=$1
    shift

    buildlibrary lib$prog.a \
        -Idep/snowhouse/include \
        -d $OBJDIR/proto/libconfig.def \
        "$@"

    buildprogram $OBJDIR/$prog \
        lib$prog.a \
        libbackend.a \
        libconfig.a \
        libtestproto.a \
        libagg.a \
        libfmt.a

    echo build $OBJDIR/$prog.stamp : test $OBJDIR/$prog-debug$EXTENSION
}

encodedecodetest() {
    local format
    format=$1
    shift

    echo "build $OBJDIR/$format.encodedecode.flux.stamp : encodedecode | fluxengine$EXTENSION scripts/encodedecodetest.sh $*"
    echo "    format=$format"
    echo "    configs=$*"
    echo "    fluxx=flux"
}

buildlibrary libagg.a \
    -Idep/agg/include \
    dep/stb/stb_image_write.c \
    dep/agg/src/*.cpp

case "$(uname)" in
    Darwin)
        buildlibrary libusbp.a \
            -Idep/libusbp/include \
            -Idep/libusbp/src \
            dep/libusbp/src/*.c \
            dep/libusbp/src/mac/*.c
        ;;

    MINGW*)
        buildlibrary libusbp.a \
            -Idep/libusbp/include \
            -Idep/libusbp/src \
            dep/libusbp/src/*.c \
            dep/libusbp/src/windows/*.c
        ;;

    *)
        buildlibrary libusbp.a \
            -Idep/libusbp/include \
            -Idep/libusbp/src \
            dep/libusbp/src/*.c \
            dep/libusbp/src/linux/*.c
        ;;
esac

buildlibrary libfmt.a \
    dep/fmt/format.cc \
    dep/fmt/os.cc \

buildproto libconfig.a \
    arch/agat/agat.proto \
    arch/aeslanier/aeslanier.proto \
    arch/amiga/amiga.proto \
    arch/apple2/apple2.proto \
    arch/brother/brother.proto \
    arch/c64/c64.proto \
    arch/f85/f85.proto \
    arch/fb100/fb100.proto \
    arch/ibm/ibm.proto \
    arch/macintosh/macintosh.proto \
    arch/micropolis/micropolis.proto \
    arch/mx/mx.proto \
    arch/northstar/northstar.proto \
    arch/tids990/tids990.proto \
    arch/victor9k/victor9k.proto \
    arch/zilogmcz/zilogmcz.proto \
    lib/common.proto \
    lib/config.proto \
    lib/decoders/decoders.proto \
    lib/drive.proto \
    lib/encoders/encoders.proto \
    lib/fluxsink/fluxsink.proto \
    lib/fluxsource/fluxsource.proto \
    lib/imagereader/imagereader.proto \
    lib/imagewriter/imagewriter.proto \
    lib/mapper.proto \
    lib/usb/usb.proto \

buildproto libfl2.a \
    lib/fl2.proto

buildlibrary libbackend.a \
    -I$OBJDIR/proto \
    -Idep/libusbp/include \
    -d $OBJDIR/proto/libconfig.def \
    -d $OBJDIR/proto/libfl2.def \
    arch/agat/agat.cc \
    arch/agat/decoder.cc \
    arch/aeslanier/decoder.cc \
    arch/amiga/amiga.cc \
    arch/amiga/decoder.cc \
    arch/amiga/encoder.cc \
    arch/apple2/decoder.cc \
    arch/apple2/encoder.cc \
    arch/brother/decoder.cc \
    arch/brother/encoder.cc \
    arch/c64/decoder.cc \
    arch/c64/encoder.cc \
    arch/f85/decoder.cc \
    arch/fb100/decoder.cc \
    arch/ibm/decoder.cc \
    arch/ibm/encoder.cc \
    arch/macintosh/decoder.cc \
    arch/macintosh/encoder.cc \
    arch/micropolis/decoder.cc \
    arch/micropolis/encoder.cc \
    arch/mx/decoder.cc \
    arch/northstar/decoder.cc \
    arch/northstar/encoder.cc \
    arch/tids990/decoder.cc \
    arch/tids990/encoder.cc \
    arch/victor9k/decoder.cc \
    arch/victor9k/encoder.cc \
    arch/zilogmcz/decoder.cc \
    lib/bitmap.cc \
    lib/bytes.cc \
    lib/crc.cc \
    lib/csvreader.cc \
    lib/decoders/decoders.cc \
    lib/decoders/fluxdecoder.cc \
    lib/decoders/fluxmapreader.cc \
    lib/decoders/fmmfm.cc \
    lib/encoders/encoders.cc \
    lib/flags.cc \
    lib/fluxmap.cc \
    lib/fluxsink/aufluxsink.cc \
    lib/fluxsink/fl2fluxsink.cc \
    lib/fluxsink/fluxsink.cc \
    lib/fluxsink/hardwarefluxsink.cc \
    lib/fluxsink/scpfluxsink.cc \
    lib/fluxsink/vcdfluxsink.cc \
    lib/fluxsource/cwffluxsource.cc \
    lib/fluxsource/erasefluxsource.cc \
    lib/fluxsource/fl2fluxsource.cc \
    lib/fluxsource/fluxsource.cc \
    lib/fluxsource/hardwarefluxsource.cc \
    lib/fluxsource/kryoflux.cc \
    lib/fluxsource/kryofluxfluxsource.cc \
    lib/fluxsource/scpfluxsource.cc \
    lib/fluxsource/testpatternfluxsource.cc \
    lib/globals.cc \
    lib/hexdump.cc \
    lib/image.cc \
    lib/imagereader/d64imagereader.cc \
    lib/imagereader/diskcopyimagereader.cc \
    lib/imagereader/imagereader.cc \
    lib/imagereader/imdimagereader.cc \
    lib/imagereader/imgimagereader.cc \
    lib/imagereader/jv3imagereader.cc \
    lib/imagereader/nsiimagereader.cc \
    lib/imagereader/td0imagereader.cc \
    lib/imagereader/dimimagereader.cc \
    lib/imagereader/fdiimagereader.cc \
    lib/imagereader/d88imagereader.cc \
    lib/imagewriter/d64imagewriter.cc \
    lib/imagewriter/diskcopyimagewriter.cc \
    lib/imagewriter/imagewriter.cc \
    lib/imagewriter/imgimagewriter.cc \
    lib/imagewriter/ldbsimagewriter.cc \
    lib/imagereader/nfdimagereader.cc \
    lib/imagewriter/nsiimagewriter.cc \
    lib/imagewriter/rawimagewriter.cc \
    lib/imginputoutpututils.cc \
    lib/ldbs.cc \
    lib/logger.cc \
    lib/mapper.cc \
    lib/proto.cc \
    lib/reader.cc \
    lib/sector.cc \
    lib/usb/fluxengineusb.cc \
    lib/usb/greaseweazle.cc \
    lib/usb/greaseweazleusb.cc \
    lib/usb/serial.cc \
    lib/usb/usb.cc \
    lib/usb/usbfinder.cc \
    lib/utils.cc \
    lib/writer.cc \

FORMATS="\
    acornadfs \
    acorndfs \
    agat840 \
    aeslanier \
    amiga \
    ampro \
    apple2 \
    appledos \
    atarist360 \
    atarist370 \
    atarist400 \
    atarist410 \
    atarist720 \
    atarist740 \
    atarist800 \
    atarist820 \
    brother120 \
    brother240 \
    commodore1541 \
    commodore1581 \
    eco1 \
    f85 \
    fb100 \
    hp9121 \
    hplif770 \
    ibm \
    ibm1200_525 \
    ibm1232 \
    ibm1440 \
    ibm180_525 \
    ibm360_525 \
    ibm720 \
    ibm720_525 \
    mac400 \
    mac800 \
    micropolis143 \
    micropolis287 \
    micropolis315 \
    micropolis630 \
    mx \
    n88basic \
    northstar175 \
    northstar350 \
    northstar87 \
    prodos \
    rx50 \
    tids990 \
    vgi \
    victor9k_ss \
    victor9k_ds \
    zilogmcz \
    "

for pb in $FORMATS; do
    buildencodedproto $OBJDIR/proto/libconfig.def ConfigProto \
        formats_${pb}_pb src/formats/$pb.textpb $OBJDIR/proto/src/formats/$pb.cc
done

buildmktable formats $OBJDIR/formats.cc $FORMATS

buildlibrary libformats.a \
    -I$OBJDIR/proto \
    -d $OBJDIR/proto/libconfig.def \
    $(for a in $FORMATS; do echo $OBJDIR/proto/src/formats/$a.cc; done) \
    $OBJDIR/formats.cc \

buildlibrary libfrontend.a \
    -I$OBJDIR/proto \
    -d $OBJDIR/proto/libconfig.def \
    src/fe-analysedriveresponse.cc \
    src/fe-analyselayout.cc \
    src/fe-inspect.cc \
    src/fe-rawread.cc \
    src/fe-rawwrite.cc \
    src/fe-read.cc \
    src/fe-rpm.cc \
    src/fe-seek.cc \
    src/fe-testbandwidth.cc \
    src/fe-testvoltages.cc \
    src/fe-write.cc \
    src/fluxengine.cc \

buildlibrary libgui.a \
    -I$OBJDIR/proto \
    -Idep/libusbp/include \
    -d $OBJDIR/proto/libconfig.def \
    src/gui/main.cc \
    src/gui/layout.cpp \
    src/gui/visualisation.cc \
    src/gui/mainwindow.cc \

buildprogram fluxengine \
    libfrontend.a \
    libformats.a \
    libbackend.a \
    libconfig.a \
    libfl2.a \
    libusbp.a \
    libfmt.a \
    libagg.a \

buildprogram fluxengine-gui \
    -rule linkgui \
    libgui.a \
    libformats.a \
    libbackend.a \
    libconfig.a \
    libfl2.a \
    libusbp.a \
    libfmt.a \

buildlibrary libemu.a \
    dep/emu/fnmatch.c

buildsimpleprogram brother120tool \
    -Idep/emu \
    tools/brother120tool.cc \
    libbackend.a \
    libemu.a \
    libfmt.a \

buildsimpleprogram brother240tool \
    -Idep/emu \
    tools/brother240tool.cc \
    libbackend.a \
    libemu.a \
    libfmt.a \

buildsimpleprogram upgrade-flux-file \
    -Idep/emu \
    tools/upgrade-flux-file.cc \
    libbackend.a \
    libfl2.a \
    libconfig.a \
    libemu.a \
    libfmt.a \

buildproto libtestproto.a \
    -d $OBJDIR/proto/lib/common.pb.h \
    tests/testproto.proto \

buildencodedproto $OBJDIR/proto/libtestproto.def TestProto testproto_pb tests/testproto.textpb $OBJDIR/proto/tests/testproto.cc

runtest agg-test            tests/agg.cc
runtest amiga-test          tests/amiga.cc
runtest bitaccumulator-test tests/bitaccumulator.cc
runtest bytes-test          tests/bytes.cc
runtest compression-test    tests/compression.cc
runtest csvreader-test      tests/csvreader.cc
runtest flags-test          tests/flags.cc
runtest fluxmapreader-test  tests/fluxmapreader.cc
runtest fluxpattern-test    tests/fluxpattern.cc
runtest fmmfm-test          tests/fmmfm.cc
runtest greaseweazle-test   tests/greaseweazle.cc
runtest kryoflux-test       tests/kryoflux.cc
runtest ldbs-test           tests/ldbs.cc
runtest utils-test          tests/utils.cc
runtest proto-test          -I$OBJDIR/proto \
                            -d $OBJDIR/proto/libconfig.def \
                            -d $OBJDIR/proto/libtestproto.def \
                            tests/proto.cc \
                            $OBJDIR/proto/tests/testproto.cc

encodedecodetest amiga
encodedecodetest apple2
encodedecodetest atarist360
encodedecodetest atarist370
encodedecodetest atarist400
encodedecodetest atarist410
encodedecodetest atarist720
encodedecodetest atarist740
encodedecodetest atarist800
encodedecodetest atarist820
encodedecodetest brother120
encodedecodetest brother240
encodedecodetest commodore1541 scripts/commodore1541_test.textpb
encodedecodetest commodore1581
encodedecodetest hp9121
encodedecodetest ibm1200_525
encodedecodetest ibm1232
encodedecodetest ibm1440
encodedecodetest ibm180_525
encodedecodetest ibm360_525
encodedecodetest ibm720
encodedecodetest ibm720_525
encodedecodetest mac400 scripts/mac400_test.textpb
encodedecodetest mac800 scripts/mac800_test.textpb
encodedecodetest n88basic
encodedecodetest rx50
encodedecodetest tids990
encodedecodetest victor9k_ss
encodedecodetest victor9k_ds

# vim: sw=4 ts=4 et

