.PHONY: all corpus
all:
	bazel test //javatests/...
	bazel build //:fluxengine

corpus:
	bazel test //:corpus

