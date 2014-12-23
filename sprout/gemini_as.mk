# gemini-as Makefile

all: stage-build

ROOT := $(abspath $(shell pwd)/../)
MK_DIR := ${ROOT}/mk

TARGET := gemini-as.so
TARGET_TEST := gemini-as.so_test

TARGET_SOURCES := mobiletwinned.cpp	\
                  sproutletappserver.cpp \
                  geminiasplugin.cpp

CPPFLAGS += -Wno-write-strings \
            -ggdb3 -std=c++0x

#	Build location independent code for shared object
CPPFLAGS += -fpic
CPPFLAGS += -I${ROOT}/include \
            -I/usr/share/clearwater/common/include \
            -I${ROOT}/modules/cpp-common/include \
            -I${ROOT}/modules/app-servers/include \
            -I${ROOT}/usr/include \
            -I${ROOT}/modules/gemini/include \
            -I${ROOT}/modules/rapidjson/include

CPPFLAGS += $(shell PKG_CONFIG_PATH=${ROOT}/usr/lib/pkgconfig pkg-config --cflags libpjproject)

# Add gemini/src as VPATH so build will find modules there.
VPATH = ${ROOT}/modules/gemini/src

# Production build:
#
# Enable optimization in production only.
CPPFLAGS := $(filter-out -O2,$(CPPFLAGS))
CPPFLAGS_BUILD += -O2

LDFLAGS += -L${ROOT}/usr/lib -L/usr/share/clearwater/common/lib -shared

include ${MK_DIR}/platform.mk

.PHONY: stage-build
stage-build: build

.PHONY: distclean
distclean: clean
