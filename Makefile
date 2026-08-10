.PHONY: all
all:
	bazel test //javatests/...
	bazel build //:fluxengine //:fluxengine_native

