#
# This is a project Makefile. It is assumed the directory this Makefile resides in is a
# project subdirectory.
#

PROJECT_NAME := ESP32_SD_SOUND_TEST

CFLAGS+=-DCONFIG_AUDIO_MAD

a:=$(shell pwd)
b:=$(dir $(patsubst %/,%,$(dir $(a))))
EXTRA_COMPONENT_DIRS:=$(b)components
	

include $(IDF_PATH)/make/project.mk


