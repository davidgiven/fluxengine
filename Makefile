all: .obj/build.ninja
	@ninja -C .obj
	@cp .obj/fluxclient fluxclient

.obj/build.ninja:
	meson .obj
