#   Copyright (C) 2014, International Business Machines
#   Corporation and others.  All Rights Reserved.
# to use the apputil library,
#  1. include this .mk file
#  2.  OBJECTS+=$(APPUTIL_OBJECTS)
#  3.  CPPFLAGS+=$(APPUTIL_CPPFLAGS)

APPUTIL_DIR=$(top_srcdir)/apputil
APPUTIL_CPPFLAGS=-I$(APPUTIL_DIR)
APPUTIL_OBJS=demoutil.o tmplutil.o
APPUTIL_OBJECTS=$(APPUTIL_OBJS:%=$(APPUTIL_DIR)/%)

OBJECTS += $(APPUTIL_OBJECTS)
CPPFLAGS += $(APPUTIL_CPPFLAGS)
