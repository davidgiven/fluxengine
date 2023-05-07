FORMATS = \
	40track_drive \
	acornadfs \
	acorndfs \
	aeslanier \
	agat \
	amiga \
	ampro \
	apple2_drive \
	apple2 \
	atarist \
	bk \
	brother \
	commodore \
	eco1 \
	epsonpf10 \
	f85 \
	fb100 \
	hplif \
	ibm \
	icl30 \
	mac \
	micropolis \
	mx \
	n88basic \
	northstar \
	psos \
	rolandd20 \
	rx50 \
	shugart_drive \
	smaky6 \
	tids990 \
	tiki \
	victor9k \
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


$(OBJDIR)/mkdoc.exe: $(OBJDIR)/scripts/mkdoc.o

$(OBJDIR)/scripts/mkdoc.o: scripts/mkdoc.cc
	@mkdir -p $(dir $@)
	@echo CXX $< $*
	@$(CXX) $(CFLAGS) -DPROTO=$* $(CXXFLAGS) -MMD -MP -MF $(@:.o=.d) -c -o $@ $<

$(call use-library, $(OBJDIR)/mkdoc.exe, $(OBJDIR)/scripts/mkdoc.o, PROTO)
$(call use-library, $(OBJDIR)/mkdoc.exe, $(OBJDIR)/scripts/mkdoc.o, LIBFORMATS)
$(call use-library, $(OBJDIR)/mkdoc.exe, $(OBJDIR)/scripts/mkdoc.o, LIBFLUXENGINE)


docs: $(patsubst %, doc/disk-%.md, $(FORMATS))

doc/disk-%.md: src/formats/%.textpb $(OBJDIR)/mkdoc.exe
	@echo MKDOC $@
	@mkdir -p $(dir $@)
	@$(OBJDIR)/mkdoc.exe $* > $@



