.PHONY: all corpus
all:
	bazel test //javatests/...
	bazel build //:fluxengine //:fluxengine_deb //:fluxengine_rpm

corpus:
	bazel test //:corpus

