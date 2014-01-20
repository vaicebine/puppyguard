AUTOMAKE_OPTIONS = foreign

ACLOCAL_AMFLAGS = -I m4

AM_MAKEFLAGS = --no-print-directory

AM_CFLAGS = -O2 -Wall -g -Werror -Wformat-security -Wextra -Iinclude/ \
		      `xml2-config --cflags` -DSYS_CONF_DIR='"$(sysconfdir)"' \
		      -Ishine/

noinst_LTLIBRARIES = libshine.la
libshine_la_CFLAGS = -fno-exceptions -Wall -O2 -fsigned-char
libshine_la_LDFLAGS = -lm -no-undefined -version-info 3:0:0
libshine_la_SOURCES  = shine/bitstream.c shine/formatbits.c shine/huffman.c \
			shine/l3bitstream.c shine/l3loop.c shine/l3mdct.c  \
			shine/l3subband.c shine/layer3.c shine/reservoir.c \
			shine/tables.c 
src_puppyguard_LDADD =  libshine.la

if OMX
OMX_CFLAGS = -DSTANDALONE -D__STDC_CONSTANT_MACROS -D__STDC_LIMIT_MACROS -DTARGET_POSIX \
		-D_LINUX -fPIC -DPIC -D_REENTRANT -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 -U_FORTIFY_SOURCE \
		-DHAVE_LIBOPENMAX=2 -DOMX -DOMX_SKIP64BIT -ftree-vectorize -pipe -DUSE_EXTERNAL_OMX -DHAVE_LIBBCM_HOST \
		-DUSE_EXTERNAL_LIBBCM_HOST -DUSE_VCHIQ_ARM -Wno-psabi \
		-I/opt/vc/include/ -I/opt/vc/include/interface/vcos/pthreads -I/opt/vc/include/interface/vmcs_host/linux \
		-I/opt/vc/src/hello_pi/libs/ilclient
AM_CFLAGS += $(OMX_CFLAGS)
AM_LDFLAGS = -lopenmaxil -lbcm_host -lvcos -lvchiq_arm -lpthread -lrt -lilclient \
		-L/opt/vc/lib/ -L/opt/vc/src/hello_pi/libs/ilclient
endif

bin_PROGRAMS = src/puppyguard
src_puppyguard_SOURCES = src/main.c src/v4l2.c src/sigfd.c \
			src/http.c src/iofd.c src/video.c src/cfg.c src/am.c \
			src/alsa.c src/audio.c src/codecmp3.c
if OMX
src_puppyguard_SOURCES += src/codecomx.c               
else
src_puppyguard_SOURCES += src/codeclib.c src/codecjpeg.c
endif



puppyguardconfdir=$(sysconfdir)/puppyguard
puppyguardconf_DATA = puppyguard.cfg

