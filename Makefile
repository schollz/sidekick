
# Project Name
TARGET = sidekick

USE_DAISYSP_LGPL = 1

# APP_TYPE=BOOT_QSPI
# without BOOT_SRAM: make program
# with BOOT_SRAM:
# hold boot, press reset and then
# > make program-boot
# press reset and then quickly press boot
# > make program-dfu

# Sources
CPP_SOURCES = main.cpp 


# Library Locations
LIBDAISY_DIR = libDaisy
DAISYSP_DIR = DaisySP
USE_FATFS = 1
LDFLAGS += -u _printf_float

# Core location, and generic Makefile.
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
include $(SYSTEM_FILES_DIR)/Makefile

.venv:
	uv venv 
	uv pip install -r requirements.txt
