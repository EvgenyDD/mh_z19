#
#Executable name
#
EXE_NAME=ctrl

#
#Folder to place compiled objects
#
BUILDDIR = build

#
#Verbose mode
#
VERBOSE=no

#
#Colorize ouput
#
COLORIZE=no

#
#Enable binary creation
#
MAKE_BINARY=yes

#
#Enable executable creation
#
MAKE_EXECUTABLE=yes

#
#Enable shared library creation
#
MAKE_SHARED_LIB=no

#
#Enable static library creation
#
MAKE_STATIC_LIB=no

#
#Enable MAP-file creation
#
CREATE_MAP=yes

#
#Tool-chain prefix
#
TCHAIN = arm-none-eabi-

#
#CPU specific options
#
MCPU += -mcpu=cortex-m4
MCPU += -mthumb
MCPU += -mfpu=fpv4-sp-d16
MCPU += -mfloat-abi=hard


#
#C language dialect
#
CDIALECT = gnu99

#
#C++ language dialect
#
CXXDIALECT = c++0x

#
#Optimization
#
OPT_LVL = 2

#
#Debug information options
#
DBG_OPTS = -gdwarf-2 -ggdb -g

#
#Additional C flags
#
CFLAGS   += -fdata-sections -ffunction-sections 
CFLAGS   += -fsingle-precision-constant

CFLAGS   += -Wno-pointer-sign
CFLAGS   += -fmessage-length=0
CFLAGS   += -fno-exceptions
CFLAGS   += -fno-move-loop-invariants -ffreestanding

CFLAGS   += -Wall -Wextra -Wstrict-prototypes  -Wdisabled-optimization -Wformat=2 -Winit-self -Wlogical-op -Wmissing-include-dirs
CFLAGS   += -Wshadow -Wsign-conversion -Wstrict-overflow=5 -Wswitch-default -Wno-unused
CFLAGS   += -pedantic -Wcast-qual -Wcast-align

# CFLAGS   += -Werror
# CFLAGS   += -Wundef 
# CFLAGS   += -Wmissing-declarations 
# CFLAGS   += -Wredundant-decls

#
#Additional CPP flags
#
CXXFLAGS += -fdata-sections -ffunction-sections 
CXXFLAGS += -fsingle-precision-constant

#
#Additional linker flags
#
# LDFLAGS  += -nostartfiles
LDFLAGS  += -specs=nano.specs
LDFLAGS  += -Wl,--gc-sections
# LDFLAGS  += -u _printf_float
# LDFLAGS  += -u _scanf_float
LDFLAGS  += -Wl,--print-memory-usage

#
#Additional link objects
#
#EXT_OBJECTS +=

#
#Additional static libraries
#
# EXT_LIBS +=c
# EXT_LIBS +=m
# EXT_LIBS +=nosys

#
#Preprocessor definitions
#
PPDEFS  += USE_HAL_DRIVER 
PPDEFS  += STM32F302x8

#
#Include directories
#
INCDIR  += ../lib
INCDIR  += src
INCDIR  += base/Inc 
INCDIR  += base/Drivers/STM32F3xx_HAL_Driver/Inc 
INCDIR  += base/Drivers/STM32F3xx_HAL_Driver/Inc/Legacy 
INCDIR  += base/Drivers/CMSIS/Device/ST/stm32f3xx/Include 
INCDIR  += base/Drivers/CMSIS/Include
INCDIR  += base/Middlewares/ST/STM32_USB_Device_Library/Core/Inc/
INCDIR  += base/Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Inc

#
#Source files
#
SOURCES += base/startup_stm32f302x8.s

SOURCES += base/Drivers/STM32F3xx_HAL_Driver/Src/stm32f3xx_hal.c 
SOURCES += base/Drivers/STM32F3xx_HAL_Driver/Src/stm32f3xx_hal_cortex.c 
SOURCES += base/Drivers/STM32F3xx_HAL_Driver/Src/stm32f3xx_hal_adc.c 
SOURCES += base/Drivers/STM32F3xx_HAL_Driver/Src/stm32f3xx_hal_adc_ex.c 
SOURCES += base/Drivers/STM32F3xx_HAL_Driver/Src/stm32f3xx_hal_crc.c 
SOURCES += base/Drivers/STM32F3xx_HAL_Driver/Src/stm32f3xx_hal_crc_ex.c 
SOURCES += base/Drivers/STM32F3xx_HAL_Driver/Src/stm32f3xx_hal_i2c.c 
SOURCES += base/Drivers/STM32F3xx_HAL_Driver/Src/stm32f3xx_hal_i2c_ex.c 
SOURCES += base/Drivers/STM32F3xx_HAL_Driver/Src/stm32f3xx_hal_rcc.c 
SOURCES += base/Drivers/STM32F3xx_HAL_Driver/Src/stm32f3xx_hal_rcc_ex.c 
SOURCES += base/Drivers/STM32F3xx_HAL_Driver/Src/stm32f3xx_hal_gpio.c 
SOURCES += base/Drivers/STM32F3xx_HAL_Driver/Src/stm32f3xx_hal_dma.c 
SOURCES += base/Drivers/STM32F3xx_HAL_Driver/Src/stm32f3xx_hal_pwr.c 
SOURCES += base/Drivers/STM32F3xx_HAL_Driver/Src/stm32f3xx_hal_pwr_ex.c 
SOURCES += base/Drivers/STM32F3xx_HAL_Driver/Src/stm32f3xx_hal_flash.c 
SOURCES += base/Drivers/STM32F3xx_HAL_Driver/Src/stm32f3xx_hal_flash_ex.
SOURCES += base/Drivers/STM32F3xx_HAL_Driver/Src/stm32f3xx_hal_exti.c 
SOURCES += base/Drivers/STM32F3xx_HAL_Driver/Src/stm32f3xx_hal_spi.c 
SOURCES += base/Drivers/STM32F3xx_HAL_Driver/Src/stm32f3xx_hal_spi_ex.c 
SOURCES += base/Drivers/STM32F3xx_HAL_Driver/Src/stm32f3xx_hal_uart.c 
SOURCES += base/Drivers/STM32F3xx_HAL_Driver/Src/stm32f3xx_hal_uart_ex.c 
SOURCES += base/Drivers/STM32F3xx_HAL_Driver/Src/stm32f3xx_hal_tim.c 
SOURCES += base/Drivers/STM32F3xx_HAL_Driver/Src/stm32f3xx_hal_tim_ex.c 
SOURCES += base/Drivers/STM32F3xx_HAL_Driver/Src/stm32f3xx_ll_usb.c 

SOURCES += base/Drivers/STM32F3xx_HAL_Driver/Src/stm32f3xx_hal_pcd.c 
SOURCES += base/Drivers/STM32F3xx_HAL_Driver/Src/stm32f3xx_hal_pcd_ex.c 
SOURCES += base/Middlewares/ST/STM32_USB_Device_Library/Core/Src/usbd_core.c
SOURCES += base/Middlewares/ST/STM32_USB_Device_Library/Core/Src/usbd_ctlreq.c
SOURCES += base/Middlewares/ST/STM32_USB_Device_Library/Core/Src/usbd_ioreq.c
SOURCES += base/Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Src/usbd_cdc.c

SOURCES += base/Src/main.c
#SOURCES += base/Src/system_stm32f3xx.c
SOURCES += base/Src/stm32f3xx_it.c
SOURCES += base/Src/stm32f3xx_hal_msp.c
SOURCES += base/Src/usb_device.c
SOURCES += base/Src/usbd_cdc_if.c
SOURCES += base/Src/usbd_conf.c
SOURCES += base/Src/usbd_desc.c


SOURCES += $(wildcard src/*.c)
SOURCES += $(wildcard ../lib/*.c)

#
#Linker scripts
#
LDSCRIPT += STM32F302C8Tx_FLASH.ld

include core.mk

.PHONY : tags
tags:
	ctags -R $(SOURCES) $(INCDIR) 