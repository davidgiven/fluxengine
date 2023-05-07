
$(OBJDIR)/tests/%.log: $(OBJDIR)/tests/%.exe
	@mkdir -p $(dir $@)
	@echo TEST $*
	@$< && touch $@

declare-test = $(eval $(declare-test-impl))
define declare-test-impl

tests: $(OBJDIR)/tests/$1.log
$(OBJDIR)/tests/$1.exe: $(OBJDIR)/tests/$1.o
$(OBJDIR)/tests/$1.o: private CFLAGS += -Idep/snowhouse/include
OBJS += $(OBJDIR)/tests/$1.o
$(call use-library, $(OBJDIR)/tests/$1.exe, $(OBJDIR)/tests/$1.o, LIBFLUXENGINE)
$(call use-library, $(OBJDIR)/tests/$1.exe, $(OBJDIR)/tests/$1.o, LIBARCH)
$(call use-library, $(OBJDIR)/tests/$1.exe, $(OBJDIR)/tests/$1.o, PROTO)
$(call use-library, $(OBJDIR)/tests/$1.exe, $(OBJDIR)/tests/$1.o, FATFS)
$(call use-library, $(OBJDIR)/tests/$1.exe, $(OBJDIR)/tests/$1.o, ADFLIB)
$(call use-library, $(OBJDIR)/tests/$1.exe, $(OBJDIR)/tests/$1.o, HFSUTILS)
$(call use-library, $(OBJDIR)/tests/$1.exe, $(OBJDIR)/tests/$1.o, LIBUSBP)

endef

$(call declare-test,agg)
$(call declare-test,amiga)
$(call declare-test,applesingle)
$(call declare-test,bitaccumulator)
$(call declare-test,bytes)
$(call declare-test,compression)
$(call declare-test,configs)
$(call declare-test,cpmfs)
$(call declare-test,csvreader)
$(call declare-test,flags)
$(call declare-test,fluxmapreader)
$(call declare-test,fluxpattern)
$(call declare-test,flx)
$(call declare-test,fmmfm)
$(call declare-test,greaseweazle)
$(call declare-test,kryoflux)
$(call declare-test,layout)
$(call declare-test,ldbs)
$(call declare-test,proto)
$(call declare-test,utils)
$(call declare-test,vfs)

$(call use-library, $(OBJDIR)/tests/agg.exe, $(OBJDIR)/tests/agg.o, AGG)
$(call use-library, $(OBJDIR)/tests/agg.exe, $(OBJDIR)/tests/agg.o, STB)
$(call use-library, $(OBJDIR)/tests/configs.exe, $(OBJDIR)/tests/configs.o, LIBFORMATS)


$(OBJDIR)/tests/proto.exe: $(OBJDIR)/tests/testproto.o
$(OBJDIR)/tests/testproto.cc: $(OBJDIR)/protoencode_TestProto.exe tests/testproto.textpb
	@mkdir -p $(dir $@)
	@echo PROTOENCODE $@
	@$^ $@ testproto_pb

