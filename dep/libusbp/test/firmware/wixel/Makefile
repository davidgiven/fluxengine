APP_NAME = test_firmware
APP_LIBS = gpio usb wixel dma adc

# Path to a Wixel SDK where the libraries have been built.
WIXEL_SDK := wixel-sdk

# Command-line utilities
CC := sdcc
MV := mv
WIXELCMD := wixelcmd

INCDIRS += $(WIXEL_SDK)/libraries/include

# C_FLAGS += -DTEST_DEVICE_B
C_FLAGS += -Wp,-MD,$(@:%.rel=%.d),-MT,$@,-MP
C_FLAGS += --disable-warning 110
C_FLAGS += $(patsubst %,-I%,$(INCDIRS))
C_FLAGS += -Wa,-p
C_FLAGS += --model-medium
LD_FLAGS += --model-medium
C_FLAGS += --debug
LD_FLAGS += --debug
LD_FLAGS += --code-loc 0x0400 --code-size 0x7400
LD_FLAGS += --iram-size 0x0100 --xram-loc 0xF000 --xram-size 0xF00
LD_FLAGS += -o $(@:%.hex=%.ihx)
LD_FLAGS += -L$(WIXEL_SDK)/libraries/lib
LD_FLAGS += $(foreach lib,$(APP_LIBS),-l$(lib))
LD_FLAGS += $(WIXEL_SDK)/libraries/xpage/xpage.rel

RELs := $(patsubst %.c,%.rel, $(wildcard *.c))

.DEFAULT_GOAL = app
.PHONY: app
app: $(APP_NAME).hex

$(APP_NAME).hex: $(RELs)
	$(CC) $(LD_FLAGS) $^
	$(MV) -f $(@:%.hex=%.ihx) $@

%.rel: %.c
	$(CC) -c $< $(C_FLAGS) -o $@

.PHONY: load
load: $(APP_NAME).hex
	$(WIXELCMD) write $<

clean:
	@rm -fv *.hex *.ihx *.rel *.d *.sym *.cdb *.mem *.rst *.map *.lst *.lk *.asm *.omf *.adb *.cdb

# Include all the dependency files generated during compilation so that Make
# knows which .rel files to recompile when a .h file changes.
-include $(RELs:%.rel=%.d)
