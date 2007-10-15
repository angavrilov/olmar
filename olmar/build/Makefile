# Makefile for olmar/build
#
#  Copyright 2006-2007 Hendrik Tews, All rights reserved. #
#  See file license.txt for terms of use                  #
###########################################################

# simply redirect all goals into the subdirectories

SUBDIRS=general astgen elsa

.PHONY: stage_1 stage_2 clean depend

stage_1:
	for d in $(SUBDIRS) ; do $(MAKE) -C $$d $@; done

stage_2:
	for d in $(SUBDIRS) ; do $(MAKE) -C $$d $@; done

clean:
	for d in $(SUBDIRS) ; do $(MAKE) -C $$d $@; done


depend:
	for d in $(SUBDIRS) ; do $(MAKE) -C $$d $@; done
