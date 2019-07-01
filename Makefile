all: .obj/build.ninja
	@ninja -C .obj test

clean:
	rm -rf .obj

.obj/build.ninja:
	@mkdir -p .obj
	meson .obj --buildtype=debugoptimized
