#   Copyright (C) 2014, International Business Machines
#   Corporation and others.  All Rights Reserved.
# to use the lxutil library,
#  1. include this .mk file
#  2. there is no step #2

LXUTIL_DIR=$(top_srcdir)/locexp/util
LXUTIL_CPPFLAGS=-I$(LXUTIL_DIR)
#LXUTIL_OBJS += fontedcb.o devanagari.o
LXUTIL_OBJS =  lx_utils.o ures_additions.o kangxi.o  lx_cpputils.o
LXUTIL_OBJS += decompcb.o
LXUTIL_OBJS += translitcb.o
LXUTIL_OBJS += cgiutil.o
LXUTIL_OBJS += unumsys.o

#LXUTIL_OBJS +=  collectcb.o translitcb.o  kannada.o
LXUTIL_OBJS += utimzone.o

LXUTIL_OBJECTS=$(LXUTIL_OBJS:%=$(LXUTIL_DIR)/%)

OBJECTS += $(LXUTIL_OBJECTS)
CPPFLAGS += $(LXUTIL_CPPFLAGS)
