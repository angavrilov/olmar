# Makefile for olmar
#
#  Copyright 2006-2007 Hendrik Tews, All rights reserved. #
#  See file license.txt for terms of use                  #
###########################################################

# simply redirect all goals into the subdirectories

SUBDIRS=build meta

.PHONY: all clean depend

all:
	for d in $(SUBDIRS) ; do $(MAKE) -C $$d $@; done


clean:
	for d in $(SUBDIRS) ; do $(MAKE) -C $$d $@; done


depend:
	for d in $(SUBDIRS) ; do $(MAKE) -C $$d $@; done

