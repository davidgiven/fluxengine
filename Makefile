.PHONY: all corpus
all:
	bazel test //...
	bazel build //:fluxengine

