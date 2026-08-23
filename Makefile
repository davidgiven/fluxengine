.PHONY: all corpus
all:
	bazel test //...
	bazel build //:fluxengine
	bazel run //java/com/cowlark/fluxengine/buildtools:mkdocindex -- $(PWD)/README.md
	rm $(PWD)/doc/disk-*.md
	bazel run //java/com/cowlark/fluxengine/buildtools:mkdoc -- $(PWD)/doc


