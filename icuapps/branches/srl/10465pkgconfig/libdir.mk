## ******************************************************************************
## *
## *   Copyright (C) 1999-2014, International Business Machines
## *   Corporation and others.  All Rights Reserved.
## *
## *******************************************************************************
## Steven R. Loomis
##
## This is a makefile for a "library" dir.
## There's not much to do, except cleanup

## Extra files to remove for 'make clean'
CLEANFILES = *~ $(DEPS)

all: all-local

install: install-local

clean: clean-local
	-rm -fv $(OBJECTS) *~ $(CLEANFILES)

distclean: clean distclean-local
	-rm -fv Makefile $(DEPS) $(EXTRA_CLEAN_FILES)

check: check-local

.PHONY: all install distclean clean check all-local install-local clean-local check-local

Makefile: $(srcdir)/Makefile.in  $(top_builddir)/config.status
	cd $(top_builddir) \
	 && CONFIG_FILES=$(subdir)/$@ CONFIG_HEADERS= $(SHELL) ./config.status

-include local.mk


ifneq ($(MAKECMDGOALS),distclean)
-include $(DEPS)
endif
