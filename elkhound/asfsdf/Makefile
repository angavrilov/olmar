# Makefile for parsgen/asfsdf

ssx.perf.txt:
	for n in 5 15 25 35 45 55 65 75 85 99; do \
	  echo -n "$$n: "; \
	  sglr -n -p SSx.trm.tbl -i ../ssx.in/$$n.in; \
	done &>$@
