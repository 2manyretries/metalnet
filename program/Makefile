#
ALT_DEVICE_FAMILY ?= soc_cv_av
SOCEDS_ROOT ?= $(SOCEDS_DEST_ROOT)
HWLIBS_ROOT = $(SOCEDS_ROOT)/ip/altera/hps/altera_hps/hwlib

HWLIBS_SRC  := alt_16550_uart.c alt_clock_manager.c alt_globaltmr.c \
  alt_timers.c alt_watchdog.c alt_cache.c alt_mmu.c alt_interrupt.c \
  alt_address_space.c
APP_SRC := metalnet.c io.c emac_core.c emac_sys.c eip.c alt_pt.c scu.c emac_phy_micrel9031.c eapp.c
ASM_SRC     := sctlr.S actlr.S
C_SRC       := $(APP_SRC) $(HWLIBS_SRC)

LINKER_SCRIPT := cycloneV-dk-ram-modified.ld

#MULTILIBFLAGS := -mcpu=cortex-a9 -mfloat-abi=softfp -mfpu=neon
MULTILIBFLAGS := -march=armv7-a -mtune=cortex-a9 -mcpu=cortex-a9 -mfloat-abi=softfp -mfpu=neon
CFLAGS  := -mno-unaligned-access -g -O0 -Wall -Werror -std=c99 $(MULTILIBFLAGS) -I$(HWLIBS_ROOT)/include -I$(HWLIBS_ROOT)/include/$(ALT_DEVICE_FAMILY) -D$(ALT_DEVICE_FAMILY)
LDFLAGS := -T$(LINKER_SCRIPT) $(MULTILIBFLAGS) -Wl,-Map=map.txt

CROSS_COMPILE := arm-altera-eabi-
CC := $(CROSS_COMPILE)gcc
LD := $(CROSS_COMPILE)g++
OC := $(CROSS_COMPILE)objcopy
MKIMAGE := mkimage
RM := rm -rf
CP := cp -f

ELF ?= $(basename $(firstword $(C_SRC))).axf
OBJ := $(patsubst %.c,%.o,$(C_SRC))
ABJ := $(patsubst %.S,%.o,$(ASM_SRC))
BIN = $(basename $(firstword $(C_SRC))).bin
IMG = $(basename $(firstword $(C_SRC)))-mkimage.bin

.PHONY: all
all: $(BIN) $(ELF) $(IMG) $(SPL)

.PHONY: clean
clean:
	$(RM) $(ELF) $(HWLIBS_SRC) $(OBJ) $(ABJ) $(BIN) $(IMG)

define SET_HWLIBS_DEPENDENCIES
$(1): $(2)
	$(CP) $(2) $(1)
endef

ALL_HWLIBS_SRC = $(wildcard $(HWLIBS_ROOT)/src/hwmgr/*.c) $(wildcard $(HWLIBS_ROOT)/src/hwmgr/$(ALT_DEVICE_FAMILY)/*.c)

$(foreach file,$(ALL_HWLIBS_SRC),$(eval $(call SET_HWLIBS_DEPENDENCIES,$(notdir $(file)),$(file))))

$(OBJ): %.o: %.c Makefile
	$(CC) $(CFLAGS) -c $< -o $@

$(ABJ): %.o: %.S Makefile
	$(CC) $(CFLAGS) -c $< -o $@

$(ELF): $(OBJ) $(ABJ)
	$(LD) $(LDFLAGS) $(OBJ) $(ABJ) -o $@
	
$(BIN): $(ELF)
	$(OC) -O binary $(ELF) $(BIN)
		
$(IMG): $(BIN)
	$(MKIMAGE) -A arm -T standalone -C none -a 0x100040 -e 0 -n "baremetal image" -d $(BIN) $(IMG)


