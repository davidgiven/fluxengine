brother120tool.exe: $(OBJDIR)/tools/brother120tool.o
brother240tool.exe: $(OBJDIR)/tools/brother240tool.o
upgrade-flux-file.exe: $(OBJDIR)/tools/upgrade-flux-file.o

$(call use-library, brother120tool.exe, $(OBJDIR)/tools/brother120tool.o, PROTO)
$(call use-library, brother120tool.exe, $(OBJDIR)/tools/brother120tool.o, LIBFLUXENGINE)
$(call use-library, brother120tool.exe, $(OBJDIR)/tools/brother120tool.o, PROTO)
$(call use-library, brother120tool.exe, $(OBJDIR)/tools/brother120tool.o, EMU)

$(call use-library, brother240tool.exe, $(OBJDIR)/tools/brother240tool.o, PROTO)
$(call use-library, brother240tool.exe, $(OBJDIR)/tools/brother240tool.o, LIBFLUXENGINE)
$(call use-library, brother240tool.exe, $(OBJDIR)/tools/brother240tool.o, EMU)

$(call use-pkgconfig, upgrade-flux-file.exe, $(OBJDIR)/tools/upgrade-flux-file.o, sqlite3)
$(call use-pkgconfig, upgrade-flux-file.exe, $(OBJDIR)/tools/upgrade-flux-file.o, zlib)
$(call use-library, upgrade-flux-file.exe, $(OBJDIR)/tools/upgrade-flux-file.o, LIBFLUXENGINE)
$(call use-library, upgrade-flux-file.exe, $(OBJDIR)/tools/upgrade-flux-file.o, PROTO)

all: brother120tool.exe brother240tool.exe upgrade-flux-file.exe

