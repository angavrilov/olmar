# Makefile for olmar
#
#  Copyright 2006-2007 Hendrik Tews, All rights reserved. #
#  See file license.txt for terms of use                  #
###########################################################

# simply redirect all goals into the subdirectories

SUBDIRS=util build meta


.PHONY: all stage_1 stage_2 clean depend

all: stage_1 stage_2


stage_1:
	for d in $(SUBDIRS) ; do $(MAKE) -C $$d $@; done


stage_2:
	for d in $(SUBDIRS) ; do $(MAKE) -C $$d $@; done


clean:
	for d in $(SUBDIRS) ; do $(MAKE) -C $$d $@; done


depend:
	for d in $(SUBDIRS) ; do $(MAKE) -C $$d $@; done


stage1: stage_1
stage2: stage_2
