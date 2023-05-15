DRIVETYPES = \
	35_40 \
	35_80 \
	525_40 \
	525_40M \
	525_80 \
	525_80M \
	8_38 \
	8_77 \
	apple2 \

$(OBJDIR)/src/gui/drivetypes/drivetype_%.o: $(OBJDIR)/src/gui/drivetypes/drivetype_%.cc
$(OBJDIR)/src/gui/drivetypes/drivetype_%.cc: $(OBJDIR)/protoencode_ConfigProto.exe src/gui/drivetypes/%.textpb
	@mkdir -p $(dir $@)
	@echo PROTOENCODE $*
	@$^ $@ drivetypes_$*_pb

$(OBJDIR)/src/gui/drivetypes/table.cc: scripts/mktable.sh src/gui/drivetypes/build.mk \
		$(patsubst %,src/gui/drivetypes/%.textpb,$(DRIVETYPES))
	@mkdir -p $(dir $@)
	@echo MKTABLE $@
	@scripts/mktable.sh drivetypes $(DRIVETYPES) > $@

LIBDRIVETYPES_SRCS = \
	$(patsubst %, $(OBJDIR)/src/gui/drivetypes/drivetype_%.cc, $(DRIVETYPES)) \
	$(OBJDIR)/src/gui/drivetypes/table.cc
LIBDRIVETYPES_OBJS = $(patsubst %.cc, %.o, $(LIBDRIVETYPES_SRCS))
.PRECIOUS: $(LIBDRIVETYPES_SRCS)

LIBDRIVETYPES_LIB = $(OBJDIR)/libgui/drivetypes.a
LIBDRIVETYPES_LDFLAGS = $(LIBDRIVETYPES_LIB)
$(LIBDRIVETYPES_LIB): $(LIBDRIVETYPES_OBJS)

