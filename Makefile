# Makefile for toplevel elsa distribution

# just do the obvious recursive thing
all:
	$(MAKE) -C smbase
	$(MAKE) -C ast
	$(MAKE) -C elkhound
	$(MAKE) -C elsa
	$(MAKE) -C asttools

check:
	$(MAKE) -C smbase check
	$(MAKE) -C ast check
	$(MAKE) -C elkhound check
	$(MAKE) -C elsa check
	$(MAKE) -C asttools check

clean:
	$(MAKE) -C smbase clean
	$(MAKE) -C ast clean
	$(MAKE) -C elkhound clean
	$(MAKE) -C elsa clean
	$(MAKE) -C asttools clean

# I generate distributions out of cvs, where there are no Makefiles
distclean:
	$(MAKE) -C smbase -f Makefile.in distclean
	$(MAKE) -C ast -f Makefile.in distclean
	$(MAKE) -C elkhound -f Makefile.in distclean
	$(MAKE) -C elsa -f Makefile.in distclean
	$(MAKE) -C asttools -f Makefile.in distclean
	rm -rf test semantic TODO

doc:
	$(MAKE) -C smbase doc
	$(MAKE) -C ast doc
	$(MAKE) -C elkhound doc
	$(MAKE) -C elsa doc
	$(MAKE) -C asttools doc
