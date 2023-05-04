FORMATS = \
	_acornadfs8 \
	_acornadfs32 \
	_apple2 \
	_atari \
	_micropolis \
	_northstar \
	_mx \
	40track_drive \
	acornadfs160 \
	acornadfs320 \
	acornadfs640 \
	acornadfs800 \
	acornadfs1600 \
	acorndfs \
	aeslanier \
	agat840 \
	amiga \
	ampro400 \
	ampro800 \
	apple2_drive \
	appleii140 \
	appleii640 \
	atarist360 \
	atarist370 \
	atarist400 \
	atarist410 \
	atarist720 \
	atarist740 \
	atarist800 \
	atarist820 \
	bk800 \
	brother120 \
	brother240 \
	commodore1541 \
	commodore1581 \
	cmd_fd2000 \
	eco1 \
	epsonpf10 \
	f85 \
	fb100 \
	hp9121 \
	hplif616 \
	hplif770 \
	ibm \
	ibm1200 \
	ibm1232 \
	ibm1440 \
	ibm180 \
	ibm160 \
	ibm360 \
	ibm320 \
	ibm720 \
	icl30 \
	mac400 \
	mac800 \
	micropolis143 \
	micropolis287 \
	micropolis315 \
	micropolis630 \
	mx110 \
	mx220_ds \
	mx220_ss \
	mx440 \
	n88basic \
	northstar175 \
	northstar350 \
	northstar87 \
	psos800 \
	rolandd20 \
	rx50 \
	shugart_drive \
	smaky6 \
	tids990 \
	tiki90 \
	tiki200 \
	tiki400 \
	tiki800 \
	victor9k_ds \
	victor9k_ss \
	zilogmcz \

$(OBJDIR)/src/formats/format_%.o: $(OBJDIR)/src/formats/format_%.cc
$(OBJDIR)/src/formats/format_%.cc: $(OBJDIR)/protoencode_ConfigProto.exe src/formats/%.textpb
	@mkdir -p $(dir $@)
	@echo PROTOENCODE $*
	@$^ $@ formats_$*_pb

OBJS += $(patsubst %, $(OBJDIR)/src/formats/format_%.o, $(FORMATS))

$(OBJDIR)/src/formats/table.cc: scripts/mktable.sh src/formats/build.mk
	@mkdir -p $(dir $@)
	@echo MKTABLE $@
	@scripts/mktable.sh formats $(FORMATS) > $@

LIBFORMATS_SRCS = \
	$(patsubst %, $(OBJDIR)/src/formats/format_%.cc, $(FORMATS)) \
	$(OBJDIR)/src/formats/table.cc
LIBFORMATS_OBJS = $(patsubst %.cc, %.o, $(LIBFORMATS_SRCS))
.PRECIOUS: $(LIBFORMATS_SRCS)

LIBFORMATS_LIB = $(OBJDIR)/libformats.a
LIBFORMATS_LDFLAGS = $(LIBFORMATS_LIB)
$(LIBFORMATS_LIB): $(LIBFORMATS_OBJS)

