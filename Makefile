#
# This is a project Makefile. It is assumed the directory this Makefile resides in is a
# project subdirectory.
#

PROJECT_NAME := AoiHashi

EXTRA_CXXFLAGS = -std=gnu++17
CPPFLAGS += -I"$(IDF_PATH)/components/bt/bluedroid/"
CPPFLAGS += -I"$(IDF_PATH)/components/bt/bluedroid/api/include/"
CPPFLAGS += -I"$(IDF_PATH)/components/bt/bluedroid/common/include/"
CPPFLAGS += -I"$(IDF_PATH)/components/bt/bluedroid/osi/include/"
CPPFLAGS += -I"$(IDF_PATH)/components/bt/bluedroid/stack/include/"
CPPFLAGS += -I"$(IDF_PATH)/components/bt/bluedroid/stack/rfcomm/include/"

include $(IDF_PATH)/make/project.mk

