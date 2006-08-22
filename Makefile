# Makefile for toplevel elsa distribution

# just do the obvious recursive thing
all:
	$(MAKE) -C smbase
	$(MAKE) -C ast
	$(MAKE) -C elkhound
	$(MAKE) -C elsa
	$(MAKE) -C semantic

check:
	$(MAKE) -C smbase check
	$(MAKE) -C ast check
	$(MAKE) -C elkhound check
	$(MAKE) -C elsa check
	$(MAKE) -C semantic check

clean:
	$(MAKE) -C smbase clean
	$(MAKE) -C ast clean
	$(MAKE) -C elkhound clean
	$(MAKE) -C elsa clean
	$(MAKE) -C semantic clean

distclean:
	$(MAKE) -C smbase distclean
	$(MAKE) -C ast distclean
	$(MAKE) -C elkhound distclean
	$(MAKE) -C elsa distclean
	$(MAKE) -C semantic distclean
	rm -rf test

doc:
	$(MAKE) -C smbase doc
	$(MAKE) -C ast doc
	$(MAKE) -C elkhound doc
	$(MAKE) -C elsa doc
	$(MAKE) -C semantic doc
