dnl -*- autoconf -*-
dnl aclocal.m4 for ICU apps
dnl Copyright (c) 2002-2014, International Business Machines Corporation and
dnl others. All Rights Reserved.

dnl @TOP@


dnl CHECK_ICU_CONFIG
AC_DEFUN([CHECK_ICU_CONFIG], [
dnl look for the icu-config script.
dnl example shown for apps that might want --without-icu as an option.
dnl sets ICU_CONFIG and ICU_VERSION

icu_path=
icu_path_extra=${prefix}/bin
ICU_VERSION=unknown

dnl for pkg-config.
PKG_PROG_PKG_CONFIG([0])
PKG_CHECK_MODULES(ICU, [icu-uc,icu-i18n,icu-io], [ICU_OK=yes], [ICU_OK=no])
if test "$ICU_OK" = "yes"; then
   dnl ICU found via pkg-config - don't try for icu-config unless requested.
   TRY_ICU_CONFIG=no
else
   dnl ICU not found via pkg-config - DO try icu-config.
   TRY_ICU_CONFIG=yes
fi
AC_ARG_WITH(icu,
	[  --with-icu, --with-icu=yes, or --with-icu={path} specify an installation containing icu-config of an installed ICU to compile against [default=yes] - ICU autodetected from pkg-config or PATH],
	[case "${withval}" in
		no) AC_MSG_ERROR([Error: These are ICU apps... --with-icu=no  / --without-icu don't make sense.])  ;;
                yes)  TRY_ICU_CONFIG=yes;;
		*) TRY_ICU_CONFIG=yes; icu_path=${withval}; icu_path_extra=${icu_path}/bin ;;
		esac],
        [icu_path=])

 AC_PATH_PROGS(ICU_CONFIG, icu-config, :, ${icu_path_extra})

 if test "$TRY_ICU_CONFIG" = "yes" -a -n "$ac_cv_path_ICU_CONFIG"; then
    AC_MSG_CHECKING([ICU installation via icu-config])
    if ${ICU_CONFIG} --exists; then
        AC_MSG_RESULT([ok])
    else
        AC_MSG_ERROR([ICU is not installed properly.])
    fi
    AC_MSG_CHECKING([ICU version via icu-config])
    ICU_VERSION=`${ICU_CONFIG} --version`
    AC_MSG_RESULT([${ICU_VERSION}])
    AC_SUBST(ICU_VERSION)
    ICU_CONFIG_TRUE=""
    ICU_CONFIG_FALSE="#"
 else
    if test "$ICU_OK" = "yes"; then
       AC_MSG_CHECKING([ICU version via pkg-config])
       ICU_VERSION=`${PKG_CONFIG} --modversion icu-uc`
       AC_SUBST(ICU_VERSION)
       AC_MSG_RESULT([${ICU_VERSION}])
       ICU_CONFIG_TRUE="#"
       ICU_CONFIG_FALSE=""
       ICU_CONFIG=
    else
       AC_MSG_ERROR([Cannot find icu-config, please check the PATH or use --with-icu=path/to/icu])
    fi
 fi
])
AC_SUBST(ICU_CONFIG)
AC_SUBST(ICU_CONFIG_FALSE)
AC_SUBST(ICU_CONFIG_TRUE) 
# ICU_CONDITIONAL - similar example taken from Automake 1.4
AC_DEFUN([ICU_CONDITIONAL],
[AC_SUBST($1_TRUE)
if $2; then
  $1_TRUE=
else
  $1_TRUE='#'
fi])
