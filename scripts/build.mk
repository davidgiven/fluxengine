$(OBJDIR)/protoencode_ConfigProto.exe: $(OBJDIR)/scripts/protoencode_ConfigProto.o
$(OBJDIR)/protoencode_TestProto.exe: $(OBJDIR)/scripts/protoencode_TestProto.o

$(OBJDIR)/scripts/protoencode_%.o: scripts/protoencode.cc
	@mkdir -p $(dir $@)
	@echo CXX $< $*
	@$(CXX) $(CFLAGS) -DPROTO=$* $(CXXFLAGS) -MMD -MP -MF $(@:.o=.d) -c -o $@ $<

$(call use-library, $(OBJDIR)/protoencode_ConfigProto.exe, $(OBJDIR)/scripts/protoencode_ConfigProto.o, PROTO)
$(call use-library, $(OBJDIR)/protoencode_TestProto.exe, $(OBJDIR)/scripts/protoencode_TestProto.o, PROTO)

