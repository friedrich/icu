#**********************************************************************
#* Copyright (C) 1999-2000, International Business Machines Corporation
#* and others.  All Rights Reserved.
#**********************************************************************
# nmake file for creating data files on win32
# invoke with
# nmake /f makedata.mak icup=<path_to_icu_instalation> [Debug|Release]
#
#	12/10/1999	weiv	Created

#If no config, we default to debug
!IF "$(CFG)" == ""
CFG=Debug
!MESSAGE No configuration specified. Defaulting to common - Win32 Debug.
!ENDIF

#Here we test if a valid configuration is given
!IF "$(CFG)" != "Release" && "$(CFG)" != "release" && "$(CFG)" != "Debug" && "$(CFG)" != "debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE
!MESSAGE NMAKE /f "makedata.mak" CFG="Debug"
!MESSAGE
!MESSAGE Possible choices for configuration are:
!MESSAGE
!MESSAGE "Release"
!MESSAGE "Debug"
!MESSAGE
!ERROR An invalid configuration is specified.
!ENDIF

#Let's see if user has given us a path to ICU
#This could be found according to the path to makefile, but for now it is this way
!IF "$(ICUP)"==""
!ERROR Can't find path!
!ENDIF
!MESSAGE icu path is $(ICUP)
RESNAME=locexp
RESDIR=.  #$(ICUP)\..\icuapps\uconv\$(RESNAME)
RESFILES=resfiles.mk
ICUDATA=$(ICUP)\data

DLL_OUTPUT=$(ICUP)\source\data
!MESSAGE ICU_DATA is not set! $(RESNAME).dll will go to $(DLL_OUTPUT)

ICD=$(ICUDATA)^\
DATA_PATH=$(ICUP)\data^\
TEST=..\source\test\testdata^\
ICUTOOLS=$(ICUP)\source\tools

# We have to prepare params for pkgdata - to help it find the tools
!IF "$(CFG)" == "Debug" || "$(CFG)" == "debug"
PKGOPT=D:$(ICUP)
!ELSE
PKGOPT=R:$(ICUP)
!ENDIF

# This appears in original Microsofts makefiles
!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE
NULL=nul
!ENDIF

PATH = $(PATH);$(ICUP)\bin

# Suffixes for data files
.SUFFIXES : .ucm .cnv .dll .dat .res .txt .c

# We're including a list of ucm files. There are two lists, one is essential 'ucmfiles.mk' and
# the other is optional 'ucmlocal.mk'
!IF EXISTS("$(RESFILES)")
!INCLUDE "$(RESFILES)"
!ELSE
!ERROR ERROR: cannot find "$(RESFILES)"
!ENDIF
RB_FILES = $(RESSRC:.txt=.res)

# This target should build all the data files
ALL : GODATA  root.txt "$(DLL_OUTPUT)\$(RESNAME).dll" GOBACK #$(RESNAME).dat
	@echo All targets are up to date

#invoke pkgdata
"$(DLL_OUTPUT)\$(RESNAME).dll" :  $(RB_FILES) $(RESFILES)
	@echo Building $(RESNAME)
 	@"$(ICUTOOLS)\pkgdata\$(CFG)\pkgdata" -v -m dll -c -p $(RESNAME) -O "$(PKGOPT)" -d "$(DLL_OUTPUT)" -s "$(RESDIR)" <<pkgdatain.txt
$(RB_FILES:.res =.res
)
<<KEEP

# utility to send us to the right dir
GODATA :
#	cd "$(RESDIR)"

root.txt: root_SAMPLE.txt
	copy root_SAMPLE.txt root.txt

# utility to get us back to the right dir
GOBACK :
#	cd "$(RESDIR)\.."

# This is to remove all the data files
CLEAN :
	@cd "$(RESDIR)"
	-@erase "*.cnv"
	-@erase "*.res"
	-@erase "$(TRANS)*.res"
	-@erase "uprops*.*"
	-@erase "unames*.*"
	-@erase "cnvalias*.*"
	-@erase "tz*.*"
	-@erase "ibm*_cnv.c"
	-@erase "*_brk.c"
	-@erase "icudata.*"
	-@erase "*.obj"
	-@erase "test*.*"
	-@erase "base*.*"
	@cd $(TEST)
	-@erase "*.res"
	@cd "$(ICUTOOLS)"

# Inference rule for creating resource bundles
.txt.res:
	@echo Making Resource Bundle files
	"$(ICUTOOLS)\genrb\$(CFG)\genrb" -s$(@D) -d$(@D) $(?F)



$(RESSRC) : {"$(ICUTOOLS)\genrb\$(CFG)"}genrb.exe

