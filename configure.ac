#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.67])
AC_INIT(ics, [1.0], [vaicebine@gmail.com])

AC_CONFIG_HEADERS([config.h])

AM_INIT_AUTOMAKE(ics, 1.0)

AC_OUTPUT(Makefile)

# Checks for programs.
AC_PROG_CC

m4_define([_LT_AC_TAGCONFIG], [])
m4_ifdef([AC_LIBTOOL_TAGS], [AC_LIBTOOL_TAGS([])])

AC_CONFIG_MACRO_DIR([m4])
AC_DISABLE_STATIC
AC_PROG_LIBTOOL


AC_ARG_ENABLE(omx, AC_HELP_STRING([--enable-omx], [enable compiling for raspberry pi]), [enable_omx=${enableval}], [enable_omx="no"])
AM_CONDITIONAL(OMX, test "${enable_omx}" != "no")

# Checks for libraries.

if (test "${enable_omx}" == "no"); then
	AC_CHECK_LIB(jpeg, jpeg_std_error, [], [AC_MSG_ERROR(jpeg library support is required)])
	AC_CHECK_HEADERS([jpeglib.h], [], [AC_MSG_ERROR(jpeglib header files are required)])
fi

AC_CHECK_LIB(ssl, SSL_library_init, [], [AC_MSG_ERROR(ssl library support is required)])

AC_CHECK_HEADERS([openssl/ssl.h], [], [AC_MSG_ERROR(ssl header files are required)])

AC_CHECK_LIB(crypto, BN_init, [], [AC_MSG_ERROR(crypto library support is required)])

AC_CHECK_LIB(asound, snd_pcm_open, [], [AC_MSG_ERROR(asound library support is required)])

AC_CHECK_HEADERS([alsa/asoundlib.h], [], [AC_MSG_ERROR(asound header files are required)])

AC_CHECK_LIB(xml2, xmlParseFile, [], [AC_MSG_ERROR(libxml2 library support is required)]) 

AC_CHECK_HEADERS([linux/videodev2.h], [], [AC_MSG_ERROR(video4linux2 header files are required)])

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h malloc.h stdint.h stdlib.h string.h sys/ioctl.h sys/time.h unistd.h], [], [AC_MSG_ERROR(header files are required)])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_INT32_T
AC_TYPE_INT8_T
AC_TYPE_SIZE_T
AC_TYPE_UINT32_T
AC_TYPE_UINT8_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_MMAP
AC_CHECK_FUNCS([munmap select strdup strerror strtol])

AC_OUTPUT
