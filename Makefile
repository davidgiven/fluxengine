.PHONY: all corpus
all:
	bazel test //javatests/...
	bazel build //:fluxengine //:fluxengine_native

corpus:
	bazel test //:corpus

