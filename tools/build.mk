$(OBJDIR)/brother120tool.exe: $(OBJDIR)/tools/brother120tool.o
$(OBJDIR)/brother240tool.exe: $(OBJDIR)/tools/brother240tool.o
$(OBJDIR)/upgrade-flux-file.exe: $(OBJDIR)/tools/upgrade-flux-file.o

OBJS += \
	$(OBJDIR)/tools/brother120tool.o \
	$(OBJDIR)/tools/brother240tool.o \
	$(OBJDIR)/tools/upgrade-flux-file.o \

$(call use-library, $(OBJDIR)/brother120tool.exe, $(OBJDIR)/tools/brother120tool.o, PROTO)
$(call use-library, $(OBJDIR)/brother120tool.exe, $(OBJDIR)/tools/brother120tool.o, LIBFLUXENGINE)
$(call use-library, $(OBJDIR)/brother120tool.exe, $(OBJDIR)/tools/brother120tool.o, LIBFORMATS)
$(call use-library, $(OBJDIR)/brother120tool.exe, $(OBJDIR)/tools/brother120tool.o, LIBUSBP)
$(call use-library, $(OBJDIR)/brother120tool.exe, $(OBJDIR)/tools/brother120tool.o, PROTO)
$(call use-library, $(OBJDIR)/brother120tool.exe, $(OBJDIR)/tools/brother120tool.o, EMU)

$(call use-library, $(OBJDIR)/brother240tool.exe, $(OBJDIR)/tools/brother240tool.o, PROTO)
$(call use-library, $(OBJDIR)/brother240tool.exe, $(OBJDIR)/tools/brother240tool.o, LIBFLUXENGINE)
$(call use-library, $(OBJDIR)/brother240tool.exe, $(OBJDIR)/tools/brother240tool.o, LIBFORMATS)
$(call use-library, $(OBJDIR)/brother240tool.exe, $(OBJDIR)/tools/brother240tool.o, LIBUSBP)
$(call use-library, $(OBJDIR)/brother240tool.exe, $(OBJDIR)/tools/brother240tool.o, EMU)

$(call use-pkgconfig, $(OBJDIR)/upgrade-flux-file.exe, $(OBJDIR)/tools/upgrade-flux-file.o, sqlite3)
$(call use-library, $(OBJDIR)/upgrade-flux-file.exe, $(OBJDIR)/tools/upgrade-flux-file.o, LIBARCH)
$(call use-library, $(OBJDIR)/upgrade-flux-file.exe, $(OBJDIR)/tools/upgrade-flux-file.o, LIBFLUXENGINE)
$(call use-library, $(OBJDIR)/upgrade-flux-file.exe, $(OBJDIR)/tools/upgrade-flux-file.o, LIBFORMATS)
$(call use-library, $(OBJDIR)/upgrade-flux-file.exe, $(OBJDIR)/tools/upgrade-flux-file.o, PROTO)
$(call use-library, $(OBJDIR)/upgrade-flux-file.exe, $(OBJDIR)/tools/upgrade-flux-file.o, LIBUSBP)

brother120tool$(EXT): $(OBJDIR)/brother120tool.exe
	@echo CP $@
	@cp $< $@

brother240tool$(EXT): $(OBJDIR)/brother240tool.exe
	@echo CP $@
	@cp $< $@

upgrade-flux-file$(EXT): $(OBJDIR)/upgrade-flux-file.exe
	@echo CP $@
	@cp $< $@

binaries: brother120tool$(EXT) brother240tool$(EXT) upgrade-flux-file$(EXT)

