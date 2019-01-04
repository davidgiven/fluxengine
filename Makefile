all: .obj/build.ninja
	@ninja -C .obj

.obj/build.ninja:
	@mkdir -p .obj
	meson .obj
