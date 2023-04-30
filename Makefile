# Makefile for the Bowdoin Shell

DRIVER = ./sdriver.pl
BSH = ./bsh
BSHREF = ./bshref
BSHARGS = "-p"
CC = gcc
CFLAGS = -Wall -Werror -g -std=gnu99
FILES = $(BSH) ./myspin ./mysplit ./mystop ./myint

all: $(FILES)

##################
# Regression tests
##################

# Run tests using the student's shell program
test01:
	$(DRIVER) -t trace01.txt -s $(BSH) -a $(BSHARGS)
test02:
	$(DRIVER) -t trace02.txt -s $(BSH) -a $(BSHARGS)
test03:
	$(DRIVER) -t trace03.txt -s $(BSH) -a $(BSHARGS)
test04:
	$(DRIVER) -t trace04.txt -s $(BSH) -a $(BSHARGS)
test05:
	$(DRIVER) -t trace05.txt -s $(BSH) -a $(BSHARGS)
test06:
	$(DRIVER) -t trace06.txt -s $(BSH) -a $(BSHARGS)
test07:
	$(DRIVER) -t trace07.txt -s $(BSH) -a $(BSHARGS)
test08:
	$(DRIVER) -t trace08.txt -s $(BSH) -a $(BSHARGS)
test09:
	$(DRIVER) -t trace09.txt -s $(BSH) -a $(BSHARGS)
test10:
	$(DRIVER) -t trace10.txt -s $(BSH) -a $(BSHARGS)
test11:
	$(DRIVER) -t trace11.txt -s $(BSH) -a $(BSHARGS)
test12:
	$(DRIVER) -t trace12.txt -s $(BSH) -a $(BSHARGS)
test13:
	$(DRIVER) -t trace13.txt -s $(BSH) -a $(BSHARGS)
test14:
	$(DRIVER) -t trace14.txt -s $(BSH) -a $(BSHARGS)

# Run the tests using the reference shell program
rtest01:
	$(DRIVER) -t trace01.txt -s $(BSHREF) -a $(BSHARGS)
rtest02:
	$(DRIVER) -t trace02.txt -s $(BSHREF) -a $(BSHARGS)
rtest03:
	$(DRIVER) -t trace03.txt -s $(BSHREF) -a $(BSHARGS)
rtest04:
	$(DRIVER) -t trace04.txt -s $(BSHREF) -a $(BSHARGS)
rtest05:
	$(DRIVER) -t trace05.txt -s $(BSHREF) -a $(BSHARGS)
rtest06:
	$(DRIVER) -t trace06.txt -s $(BSHREF) -a $(BSHARGS)
rtest07:
	$(DRIVER) -t trace07.txt -s $(BSHREF) -a $(BSHARGS)
rtest08:
	$(DRIVER) -t trace08.txt -s $(BSHREF) -a $(BSHARGS)
rtest09:
	$(DRIVER) -t trace09.txt -s $(BSHREF) -a $(BSHARGS)
rtest10:
	$(DRIVER) -t trace10.txt -s $(BSHREF) -a $(BSHARGS)
rtest11:
	$(DRIVER) -t trace11.txt -s $(BSHREF) -a $(BSHARGS)
rtest12:
	$(DRIVER) -t trace12.txt -s $(BSHREF) -a $(BSHARGS)
rtest13:
	$(DRIVER) -t trace13.txt -s $(BSHREF) -a $(BSHARGS)
rtest14:
	$(DRIVER) -t trace14.txt -s $(BSHREF) -a $(BSHARGS)

# clean up
clean:
	rm -f $(FILES) *.o *~

