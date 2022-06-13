ifeq ($(shell $(PKG_CONFIG) fmt; echo $$?), 0)

LIBFMT_LIB =
LIBFMT_CFLAGS := $(shell $(PKG_CONFIG) --cflags fmt)
LIBFMT_LDFLAGS := $(shell $(PKG_CONFIG) --libs fmt)

else

$(error required dependency 'fmt' missing)

endif

