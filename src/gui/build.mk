ifneq ($(shell command -v $(WX_CONFIG)),)

FLUXENGINE_GUI_SRCS = \
	src/gui/main.cc \
	src/gui/mainwindow.cc \
	src/gui/visualisation.cc \
	src/gui/layout.cpp \
 
FLUXENGINE_GUI_OBJS = \
	$(patsubst %.cpp, $(OBJDIR)/%.o, \
	$(patsubst %.cc, $(OBJDIR)/%.o, $(FLUXENGINE_GUI_SRCS)) \
	)
$(FLUXENGINE_GUI_SRCS): | $(PROTO_HDRS)
$(FLUXENGINE_GUI_OBJS): CFLAGS += $(shell $(WX_CONFIG) --cxxflags core base)
fluxengine-gui.exe: LDFLAGS += $(shell $(WX_CONFIG) --libs core base)
fluxengine-gui.exe: $(FLUXENGINE_GUI_OBJS)

$(call use-library, fluxengine-gui.exe, $(FLUXENGINE_GUI_OBJS), FMT)
$(call use-library, fluxengine-gui.exe, $(FLUXENGINE_GUI_OBJS), LIBARCH)
$(call use-library, fluxengine-gui.exe, $(FLUXENGINE_GUI_OBJS), LIBFLUXENGINE)
$(call use-library, fluxengine-gui.exe, $(FLUXENGINE_GUI_OBJS), LIBFORMATS)
$(call use-library, fluxengine-gui.exe, $(FLUXENGINE_GUI_OBJS), LIBUSBP)
$(call use-library, fluxengine-gui.exe, $(FLUXENGINE_GUI_OBJS), PROTO)

all: fluxengine-gui.exe

else

$(warning wx-config missing, not building GUI)

endif

