mydir := $(call my-dir)

#
# Include the aac.decoders property:
#
include $(mydir)/../.ant.properties

AAC_DECODERS 	:=	$(aac.decoders)
LOGLEVEL 		:=	$(jni.loglevel)
MMS_WMA	 		:=	$(mms)

LOGLEVELS =
# Loglevels
ifeq ($(LOGLEVEL),error)
	LOGLEVELS	+= ERROR
endif
ifeq ($(LOGLEVEL),warn)
	LOGLEVELS	+= ERROR WARN
endif
ifeq ($(LOGLEVEL),info)
	LOGLEVELS	+= ERROR WARN INFO
endif
ifeq ($(LOGLEVEL),debug)
	LOGLEVELS	+= ERROR WARN INFO DEBUG
endif
ifeq ($(LOGLEVEL),trace)
	LOGLEVELS	+= ERROR WARN INFO DEBUG TRACE
endif

cflags_loglevels	:= $(foreach ll,$(LOGLEVELS),-DAACD_LOGLEVEL_$(ll))

include $(mydir)/aac-decoder/Android.mk

ifeq ($(MMS_WMA),yes)
	include $(mydir)/libmms/Android.mk
endif

#dump:
#	$(warning $(modules-dump-database))
#	$(warning $(dump-src-file-tags))
#	$(error Dump finished)
