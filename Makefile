all: .obj/build.ninja
	@ninja -f .obj/build.ninja

clean:
	@echo CLEAN
	@rm -rf .obj

.obj/build.ninja: mkninja.sh
	@echo MKNINJA $@
	@mkdir -p $(dir $@)
	@sh $< > $@

