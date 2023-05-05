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
	atarist \
	bk800 \
	brother \
	commodore1541 \
	commodore1581 \
	cmd_fd2000 \
	eco1 \
	epsonpf10 \
	f85 \
	fb100 \
	hplif \
	ibm \
	icl30 \
	mac400 \
	mac800 \
	micropolis \
	mx \
	n88basic \
	northstar \
	psos800 \
	rolandd20 \
	rx50 \
	shugart_drive \
	smaky6 \
	tids990 \
	tiki \
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

