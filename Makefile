export CXX = g++
export AR = ar rcs
export STRIP = strip
export CFLAGS = -Og -g --std=c++14
export LDFLAGS = -Og

export OBJDIR = .obj

all: .obj/build.ninja
	@ninja -f .obj/build.ninja

clean:
	@echo CLEAN
	@rm -rf $(OBJDIR)

.obj/build.ninja: mkninja.sh
	@echo MKNINJA $@
	@mkdir -p $(OBJDIR)
	@sh $< > $@

