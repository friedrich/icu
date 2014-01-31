#   Copyright (C) 2014, International Business Machines
#   Corporation and others.  All Rights Reserved.
# to use the usort library,
#  1. include this .mk file
#  2.  OBJECTS+=$(USORT_OBJECTS)
#  3.  CPPFLAGS+=$(USORT_CPPFLAGS)

USORT_DIR=$(top_srcdir)/usort/lib
USORT_CPPFLAGS=-I$(USORT_DIR)
USORT_OBJS=usort.o
USORT_OBJECTS=$(USORT_OBJS:%=$(USORT_DIR)/%)

OBJECTS += $(USORT_OBJECTS)
CPPFLAGS += $(USORT_CPPFLAGS)
