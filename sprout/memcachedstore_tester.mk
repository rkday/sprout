# sprout-base Makefile

all: stage-build

ROOT := $(abspath $(shell pwd)/../)
MK_DIR := ${ROOT}/mk

TARGET := memcachedstore_tester

TARGET_SOURCES := logger.cpp \
                  saslogger.cpp \
                  utils.cpp \
                  memcachedstore.cpp \
                  memcachedstoreview.cpp \
                  log.cpp \
                  statistic.cpp \
                  zmq_lvc.cpp \
                  accumulator.cpp \
                  ipv6utils.cpp \
                  alarm.cpp \
                  communicationmonitor.cpp

TARGET_SOURCES_BUILD := memcachedstore_tester.cpp

CPPFLAGS += -Wno-write-strings \
            -ggdb3 -std=c++0x -fno-access-control
CPPFLAGS += -I${ROOT}/include \
            -I${ROOT}/modules/cpp-common/include \
            -I${ROOT}/usr/include \
            -I${ROOT}/modules/rapidjson/include

CPPFLAGS += $(shell PKG_CONFIG_PATH=${ROOT}/usr/lib/pkgconfig pkg-config --cflags libpjproject)

# Add cpp-common/src as VPATH so build will find modules there.
VPATH = ${ROOT}/modules/cpp-common/src

# Production build:
#
# Enable optimization in production only.
CPPFLAGS := $(filter-out -O2,$(CPPFLAGS))
CPPFLAGS_BUILD += -O2


LDFLAGS += -L${ROOT}/usr/lib -rdynamic
LDFLAGS += -lmemcached \
           -lmemcachedutil \
           -lzmq \
           -lsas

include ${MK_DIR}/platform.mk

.PHONY: stage-build
stage-build: build

.PHONY: distclean
distclean: clean
