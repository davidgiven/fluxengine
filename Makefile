.PHONY: all corpus
all:
	bazel test //javatests/...
	bazel build //:fluxengine //:fluxengine_deb //:fluxengine_rpm //:fluxengine_app_image

corpus:
	bazel test //:corpus

