# Makefile for perimeter
# For copyright information, see olden_v1.01/COPYRIGHT

SRCS     = main.c maketree.c

CPP         = gcc -E 
CPPFLAGS    = 
CFLAGS      = $(CPPFLAGS) -Wall
OBJS        = maketree.o main.o

all: perimeter


%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

%.i: %.c
	$(CPP) $(CPPFLAGS) $< -o $@

perimeter: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o perimeter

clean:
	rm -f perimeter $(OBJS)
