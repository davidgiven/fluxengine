.PHONY: all run
all:
	bazel test //... //:fluxengine
	bazel run //java/com/cowlark/fluxengine/buildtools:mkdocindex -- $(PWD)/README.md
	rm $(PWD)/doc/disk-*.md
	bazel run //java/com/cowlark/fluxengine/buildtools:mkdoc -- $(PWD)/doc

run:
	bazel run //:fluxengine -- gui --jvm_flag=-Dorg.slf4j.simpleLogger.defaultLogLevel=info
