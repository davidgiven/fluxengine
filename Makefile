all: .obj/build.ninja
	@ninja -C .obj test

.obj/build.ninja:
	@mkdir -p .obj
	meson .obj
