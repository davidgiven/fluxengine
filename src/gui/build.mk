ifneq ($(shell sh -c "command -v $(WX_CONFIG)"),)

FLUXENGINE_GUI_SRCS = \
	src/gui/main.cc \
	src/gui/mainwindow.cc \
	src/gui/visualisation.cc \
	src/gui/layout.cpp \
 
FLUXENGINE_GUI_OBJS = \
	$(patsubst %.cpp, $(OBJDIR)/%.o, \
	$(patsubst %.cc, $(OBJDIR)/%.o, $(FLUXENGINE_GUI_SRCS)) \
	)
OBJS += $(FLUXENGINE_GUI_OBJS)
$(FLUXENGINE_GUI_SRCS): | $(PROTO_HDRS)
$(FLUXENGINE_GUI_OBJS): CFLAGS += $(shell $(WX_CONFIG) --cxxflags core base)
FLUXENGINE_GUI_BIN = $(OBJDIR)/fluxengine-gui.exe
$(FLUXENGINE_GUI_BIN): LDFLAGS += $(shell $(WX_CONFIG) --libs core base)
$(FLUXENGINE_GUI_BIN): $(FLUXENGINE_GUI_OBJS)

$(call use-pkgconfig, $(FLUXENGINE_GUI_BIN), $(FLUXENGINE_GUI_OBJS), fmt)
$(call use-library, $(FLUXENGINE_GUI_BIN), $(FLUXENGINE_GUI_OBJS), LIBARCH)
$(call use-library, $(FLUXENGINE_GUI_BIN), $(FLUXENGINE_GUI_OBJS), LIBFLUXENGINE)
$(call use-library, $(FLUXENGINE_GUI_BIN), $(FLUXENGINE_GUI_OBJS), LIBFORMATS)
$(call use-library, $(FLUXENGINE_GUI_BIN), $(FLUXENGINE_GUI_OBJS), LIBUSBP)
$(call use-library, $(FLUXENGINE_GUI_BIN), $(FLUXENGINE_GUI_OBJS), PROTO)

binaries: fluxengine-gui$(EXT)

fluxengine-gui$(EXT): $(FLUXENGINE_GUI_BIN)
	@echo CP $@
	@cp $< $@

else

$(warning wx-config missing, not building GUI)

endif

