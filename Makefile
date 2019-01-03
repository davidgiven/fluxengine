all: .obj/build.ninja
	@ninja -C .obj

.obj/build.ninja:
	meson .obj
