FORMATS = \
	40track_drive \
	acornadfs \
	acorndfs \
	aeslanier \
	agat840 \
	amiga \
	ampro \
	apple2_drive \
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
	bk800 \
	brother120 \
	brother240 \
	commodore1541 \
	commodore1581 \
	eco1 \
	f85 \
	fb100 \
	hp9121 \
	hplif770 \
	ibm1200 \
	ibm1232 \
	ibm1440 \
	ibm180 \
	ibm360 \
	ibm720 \
	ibm \
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
	shugart_drive \
	tids990 \
	vgi \
	victor9k_ds \
	victor9k_ss \
	zilogmcz \

$(OBJDIR)/protoencode.exe: $(OBJDIR)/scripts/protoencode.o
$(call use-library, $(OBJDIR)/protoencode.exe, $(OBJDIR)/scripts/protoencode.o, PROTO)

$(OBJDIR)/src/formats/format_%.o: $(OBJDIR)/src/formats/format_%.cc
$(OBJDIR)/src/formats/format_%.cc: $(OBJDIR)/protoencode.exe src/formats/%.textpb
	@mkdir -p $(dir $@)
	@echo PROTOENCODE $*
	@$^ $@ formats_$*_pb

$(OBJDIR)/src/formats/table.cc: scripts/mktable.sh
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

