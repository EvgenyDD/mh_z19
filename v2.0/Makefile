EXE_NAME=mh_z19
VER_MAJ =   1
VER_MIN =   0
VER_PATCH = 0
MAKE_BINARY=yes

TCHAIN = arm-none-eabi-
MCPU += -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb
CDIALECT = gnu99
OPT_LVL = 2
DBG_OPTS = -gdwarf-2 -ggdb -g

CFLAGS   += -fvisibility=hidden -funsafe-math-optimizations -fdata-sections -ffunction-sections -fno-move-loop-invariants
CFLAGS   += -fmessage-length=0 -fno-exceptions -fno-common -fno-builtin -ffreestanding
CFLAGS   += -fsingle-precision-constant
CFLAGS   += $(C_FULL_FLAGS)
CFLAGS   += -Werror

CXXFLAGS += -fvisibility=hidden -funsafe-math-optimizations -fdata-sections -ffunction-sections -fno-move-loop-invariants
CXXFLAGS += -fmessage-length=0 -fno-exceptions -fno-common -fno-builtin -ffreestanding
CXXFLAGS += -fvisibility-inlines-hidden -fuse-cxa-atexit -felide-constructors -fno-rtti
CXXFLAGS += -fsingle-precision-constant
CXXFLAGS += $(CXX_FULL_FLAGS)
CXXFLAGS += -Werror

LDFLAGS  += -specs=nano.specs
LDFLAGS  += -Wl,--gc-sections
LDFLAGS  += -Wl,--print-memory-usage
# LDFLAGS  += -u _printf_float

EXT_LIBS +=c m nosys

PPDEFS += USE_STDPERIPH_DRIVER STM32F302x8 FW_TYPE=FW_APP CFG_SYSTEM_SAVES_NON_NATIVE_DATA=1 DEV=\"MH-Z19\"
# PPDEFS += USBD_CLASS_DFU
# PPDEFS += USBD_CLASS_CDC
PPDEFS += USBD_CLASS_COMPOSITE_DFU_CDC
PPDEFS += DFU_READS_CFG_SECTION
PPDEFS += HSE_VALUE=12000000

INCDIR += app
INCDIR += common
INCDIR += common/CMSIS
INCDIR += common/config_system
INCDIR += common/console
INCDIR += common/crc
INCDIR += common/fw_header
INCDIR += common/md5
INCDIR += common/STM32_USB-FS-Device_Driver
INCDIR += common/STM32F30x_StdPeriph_Driver/inc
INCDIR += common/usb

SOURCES += $(wildcard common/*.c)
SOURCES += $(call rwildcard, app, *.c *.S *.s)
SOURCES += $(call rwildcard, common/CMSIS, *.c *.S *.s)
SOURCES += $(call rwildcard, common/config_system, *.c *.S *.s)
SOURCES += $(call rwildcard, common/console, *.c *.S *.s)
SOURCES += $(call rwildcard, common/crc, *.c *.S *.s)
SOURCES += $(call rwildcard, common/fw_header, *.c *.S *.s)
SOURCES += $(call rwildcard, common/md5, *.c *.S *.s)
SOURCES += $(call rwildcard, common/STM32_USB-FS-Device_Driver, *.c *.S *.s)
SOURCES += $(call rwildcard, common/STM32F30x_StdPeriph_Driver, *.c *.S *.s)
SOURCES += $(call rwildcard, common/usb, *.c *.S *.s)
SOURCES += common/startup_stm32f302x8.s

LDSCRIPT += ldscript/app.ld

BINARY_SIGNED = $(BUILDDIR)/$(EXE_NAME)_app_signed.bin
BINARY_MERGED = $(BUILDDIR)/$(EXE_NAME)_full.bin
SIGN = $(BUILDDIR)/sign
ARTEFACTS += $(BINARY_SIGNED)

PRELDR_SIGNED = preldr/build/$(EXE_NAME)_preldr_signed.bin
LDR_SIGNED = ldr/build/$(EXE_NAME)_ldr_signed.bin 
EXT_MAKE_TARGETS = $(PRELDR_SIGNED) $(LDR_SIGNED)

include common/core.mk

signer:
	@make -C signer_fw_tool -j

$(SIGN): sign/sign.c
	@gcc $< -o $@

$(BINARY_SIGNED): $(BINARY) $(SIGN)
	@$(SIGN) $(BINARY) $@ 45056 \
		prod=$(EXE_NAME) \
		prod_name=$(EXE_NAME)_app \
		ver_maj=$(VER_MAJ) \
		ver_min=$(VER_MIN) \
		ver_pat=$(VER_PATCH) \
		build_ts=$$(date -u +'%Y%m%d_%H%M%S')

$(BINARY_MERGED): $(EXT_MAKE_TARGETS) $(BINARY_SIGNED)
	@echo " [Merging binaries] ..." {$@}
	@cp -f $(PRELDR_SIGNED) $@
	@dd if=$(LDR_SIGNED) of=$@ bs=1 oflag=append seek=4096 status=none
	@dd if=$(BINARY_SIGNED) of=$@ bs=1 oflag=append seek=20480 status=none

.PHONY: composite
composite: $(BINARY_MERGED)

.PHONY: clean
clean: clean_ext_targets

.PHONY: $(EXT_MAKE_TARGETS)
$(EXT_MAKE_TARGETS):
	@$(MAKE) -C $(subst build/,,$(dir $@)) --no-print-directory

.PHONY: clean_ext_targets
clean_ext_targets:
	$(foreach var,$(EXT_MAKE_TARGETS),$(MAKE) -C $(subst build/,,$(dir $(var))) clean;)

#####################
### FLASH & DEBUG ###
#####################

flash: $(BINARY_SIGNED)
	@openocd -f target/stm32f30x.cfg -c "program $(BINARY_SIGNED) 0x08050000 verify reset exit" 

flash_all: $(BINARY_MERGED)
	@openocd -f target/stm32f30x.cfg -c "program $(BINARY_MERGED) 0x08000000 verify reset exit"

program: $(BINARY_SIGNED)
	@usb_dfu_flasher w a $< MH-Z19

ds:
	openocd -d0 -f target/stm32f30x.cfg

debug:
	@set _NO_DEBUG_HEAP=1
	@echo "file $(EXECUTABLE)" > .gdbinit
	@echo "set auto-load safe-path /" >> .gdbinit
	@echo "set confirm off" >> .gdbinit
	@echo "target extended-remote :3333" >> .gdbinit
	@arm-none-eabi-gdb -q -x .gdbinit