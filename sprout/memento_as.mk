# memento-as Makefile

all: stage-build

ROOT := $(abspath $(shell pwd)/../)
MK_DIR := ${ROOT}/mk

TARGET := memento-as.so
TARGET_TEST := memento-as.so_test

TARGET_SOURCES := sproutletappserver.cpp \
                  mementoasplugin.cpp \
                  cassandra_store.cpp \
                  call_list_store.cpp \
                  mementosaslogger.cpp \
                  call_list_store_processor.cpp \
                  mementoappserver.cpp

CPPFLAGS += -Wno-write-strings \
            -ggdb3 -std=c++0x

# Build location independent code for shared object
CPPFLAGS += -fpic
CPPFLAGS += -I${ROOT}/include \
			-I/usr/share/clearwater/common/include \
            -I${ROOT}/modules/memento/include \
            -I${ROOT}/modules/cpp-common/include \
            -I${ROOT}/modules/app-servers/include \
            -I${ROOT}/usr/include \
            -I${ROOT}/modules/rapidjson/include

CPPFLAGS += $(shell PKG_CONFIG_PATH=${ROOT}/usr/lib/pkgconfig pkg-config --cflags libpjproject)

# Add memento/src as VPATH so build will find modules there.
VPATH = ${ROOT}/modules/memento/src:${ROOT}/modules/cpp-common/src

# Production build:
#
# Enable optimization in production only.
CPPFLAGS := $(filter-out -O2,$(CPPFLAGS))
CPPFLAGS_BUILD += -O2

LDFLAGS += -L${ROOT}/usr/lib -L/usr/share/clearwater/common/lib -shared
LDFLAGS += -lthrift \
           -lcassandra

include ${MK_DIR}/platform.mk

.PHONY: stage-build
stage-build: build

.PHONY: distclean
distclean: clean
