BUILD_BASE	= build
FW_BASE		= firmware

XTENSA_TOOLS_ROOT ?= /opt/Espressif/crosstool-NG/builds/xtensa-lx106-elf/bin/

SDK_BASE ?= /opt/Espressif/ESP8266_SDK

ESPTOOL ?= /opt/Espressif/esptool-py/esptool.py
ESPPORT ?= /dev/ttyUSB0
ESP_FLASH_PARAMS = -fm dout -ff 40m -fs 1MB

TARGET	= app

# Which modules (subdirectories) to include in compiling
MODULES	= user
EXTRA_INCDIR = include

LIBS		= c gcc hal pp phy net80211 lwip wpa main

CFLAGS		= -Os -g -O2 -Wpointer-arith -Wundef -Werror -Wl,-EL -fno-inline-functions -nostdlib -mlongcalls -mtext-section-literals  -D__ets__ -DICACHE_FLASH

# linker flags used to generate main object file
LDFLAGS		= -nostdlib -Wl,--no-check-sections -u call_user_start -Wl,-static

# linker script used for the linking stage (above)
LD_SCRIPT	= eagle.app.v6.ld

SDK_LIBDIR	= lib
SDK_LDDIR	= ld
SDK_INCDIR	= include include/json

# We create two different files for uploading into the flash
FW_FILE_1_ADDR	= 0x00000
FW_FILE_2_ADDR	= 0x40000

# Tools to use as compiler, librarian and linker
CC		:= $(XTENSA_TOOLS_ROOT)/xtensa-lx106-elf-gcc
AR		:= $(XTENSA_TOOLS_ROOT)/xtensa-lx106-elf-ar
LD		:= $(XTENSA_TOOLS_ROOT)/xtensa-lx106-elf-gcc


SRC_DIR		:= $(MODULES)
BUILD_DIR	:= $(addprefix $(BUILD_BASE)/,$(MODULES))

SDK_LIBDIR	:= $(addprefix $(SDK_BASE)/,$(SDK_LIBDIR))
SDK_INCDIR	:= $(addprefix -I$(SDK_BASE)/,$(SDK_INCDIR))

SRC		:= $(foreach sdir,$(SRC_DIR),$(wildcard $(sdir)/*.c))
OBJ		:= $(patsubst %.c,$(BUILD_BASE)/%.o,$(SRC))
LIBS		:= $(addprefix -l,$(LIBS))
APP_AR		:= $(addprefix $(BUILD_BASE)/,$(TARGET)_app.a)
TARGET_OUT	:= $(addprefix $(BUILD_BASE)/,$(TARGET).out)

LD_SCRIPT	:= $(addprefix -T$(SDK_BASE)/$(SDK_LDDIR)/,$(LD_SCRIPT))

INCDIR	:= $(addprefix -I,$(SRC_DIR))
EXTRA_INCDIR	:= $(addprefix -I,$(EXTRA_INCDIR))
MODULE_INCDIR	:= $(addsuffix /include,$(INCDIR))

FW_FILE_1	:= $(addprefix $(FW_BASE)/,$(FW_FILE_1_ADDR).bin)
FW_FILE_2	:= $(addprefix $(FW_BASE)/,$(FW_FILE_2_ADDR).bin)

V ?= $(VERBOSE)
ifeq ("$(V)","1")
Q :=
vecho := @true
else
Q := @
vecho := @echo
endif

vpath %.c $(SRC_DIR)

define compile-objects
$1/%.o: %.c
	$(vecho) "CC $$<"
	$(Q) $(CC) $(INCDIR) $(MODULE_INCDIR) $(EXTRA_INCDIR) $(SDK_INCDIR) $(CFLAGS) -c $$< -o $$@
endef

.PHONY: all checkdirs flash clean

all: checkdirs $(TARGET_OUT) $(FW_FILE_1) $(FW_FILE_2)

$(FW_BASE)/%.bin: $(TARGET_OUT) | $(FW_BASE)
	$(vecho) "FW $(FW_BASE)/"
	$(Q) $(ESPTOOL) elf2image -o $(FW_BASE)/ $(TARGET_OUT)

$(TARGET_OUT): $(APP_AR)
	$(vecho) "LD $@"
	$(Q) $(LD) -L$(SDK_LIBDIR) $(LD_SCRIPT) $(LDFLAGS) -Wl,--start-group $(LIBS) $(APP_AR) -Wl,--end-group -o $@

$(APP_AR): $(OBJ)
	$(vecho) "AR $@"
	$(Q) $(AR) cru $@ $^

checkdirs: $(BUILD_DIR) $(FW_BASE)

$(BUILD_DIR):
	$(Q) mkdir -p $@

$(FW_BASE):
	$(Q) mkdir -p $@

flash: $(FW_FILE_1) $(FW_FILE_2)
	$(ESPTOOL) --port $(ESPPORT) write_flash $(ESP_FLASH_PARAMS) $(FW_FILE_1_ADDR) $(FW_FILE_1) $(FW_FILE_2_ADDR) $(FW_FILE_2)

clean:
	$(Q) rm -rf $(FW_BASE) $(BUILD_BASE)

$(foreach bdir,$(BUILD_DIR),$(eval $(call compile-objects,$(bdir))))