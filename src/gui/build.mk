ifneq ($(shell $(WX_CONFIG) --version),)

FLUXENGINE_GUI_SRCS = \
	src/gui/customstatusbar.cc \
	src/gui/filesystemmodel.cc \
	src/gui/fluxviewercontrol.cc \
	src/gui/fluxviewerwindow.cc \
	src/gui/layout.cpp \
	src/gui/main.cc \
	src/gui/mainwindow.cc \
	src/gui/texteditorwindow.cc \
	src/gui/textviewerwindow.cc \
	src/gui/fileviewerwindow.cc \
	src/gui/visualisationcontrol.cc \
 
FLUXENGINE_GUI_OBJS = \
	$(patsubst %.cpp, $(OBJDIR)/%.o, \
	$(patsubst %.cc, $(OBJDIR)/%.o, $(FLUXENGINE_GUI_SRCS)) \
	)
OBJS += $(FLUXENGINE_GUI_OBJS)
$(FLUXENGINE_GUI_SRCS): | $(PROTO_HDRS)
$(FLUXENGINE_GUI_OBJS): CFLAGS += $(shell $(WX_CONFIG) --cxxflags core base adv aui)
FLUXENGINE_GUI_BIN = $(OBJDIR)/fluxengine-gui.exe
$(FLUXENGINE_GUI_BIN): LDFLAGS += $(shell $(WX_CONFIG) --libs core base adv aui)
$(FLUXENGINE_GUI_BIN): $(FLUXENGINE_GUI_OBJS)

$(call use-pkgconfig, $(FLUXENGINE_GUI_BIN), $(FLUXENGINE_GUI_OBJS), fmt)
$(call use-library, $(FLUXENGINE_GUI_BIN), $(FLUXENGINE_GUI_OBJS), LIBARCH)
$(call use-library, $(FLUXENGINE_GUI_BIN), $(FLUXENGINE_GUI_OBJS), LIBFLUXENGINE)
$(call use-library, $(FLUXENGINE_GUI_BIN), $(FLUXENGINE_GUI_OBJS), LIBFORMATS)
$(call use-library, $(FLUXENGINE_GUI_BIN), $(FLUXENGINE_GUI_OBJS), LIBUSBP)
$(call use-library, $(FLUXENGINE_GUI_BIN), $(FLUXENGINE_GUI_OBJS), PROTO)
$(call use-library, $(FLUXENGINE_GUI_BIN), $(FLUXENGINE_GUI_OBJS), FATFS)
$(call use-library, $(FLUXENGINE_GUI_BIN), $(FLUXENGINE_GUI_OBJS), ADFLIB)
$(call use-library, $(FLUXENGINE_GUI_BIN), $(FLUXENGINE_GUI_OBJS), HFSUTILS)

binaries: fluxengine-gui$(EXT)

fluxengine-gui$(EXT): $(FLUXENGINE_GUI_BIN)
	@echo CP $@
	@cp $< $@

ifeq ($(PLATFORM),OSX)

binaries: FluxEngine.pkg

FluxEngine.pkg: FluxEngine.app
	@echo PKGBUILD $@
	@pkgbuild --quiet --install-location /Applications --component $< $@

FluxEngine.app: fluxengine-gui$(EXT) $(OBJDIR)/fluxengine.icns
	@echo MAKEAPP $@
	@rm -rf $@
	@cp -a extras/FluxEngine.app.template $@
	@cp fluxengine-gui$(EXT) $@/Contents/MacOS/fluxengine-gui
	@mkdir -p $@/Contents/Resources
	@cp $(OBJDIR)/fluxengine.icns $@/Contents/Resources/FluxEngine.icns
	@for name in `otool -L fluxengine-gui$(EXT) | tr -d '\t' | grep -v '^/System/' | grep -v '^/usr/lib/' | grep -v ':$$' | awk '{print $$1}'`; do cp "$$name" $@/Contents/Resources; done

$(OBJDIR)/fluxengine.icns: $(OBJDIR)/fluxengine.iconset
	@echo ICONUTIL $@
	@iconutil -c icns -o $@ $<

$(OBJDIR)/fluxengine.iconset: extras/icon.png 
	@echo ICONSET $@
	@rm -rf $@
	@mkdir -p $@
	@sips -z 64 64 $< --out $@/icon_32x32@2x.png > /dev/null

endif

else

$(warning wx-config missing, not building GUI)

endif

