# use normal make for this Makefile
#
# Makefile for building user programs to run on top of Nachos
#
# Several things to be aware of:
#
#    Nachos assumes that the location of the program startup routine (the
# 	location the kernel jumps to when the program initially starts up)
#       is at location 0.  This means: start.o must be the first .o passed 
# 	to ld, in order for the routine "Start" to be loaded at location 0
#

# If you are running on SunOS 5.x machines, uncomment the next line
GCCDIR = /auto/home-scf-22/csci402/testgnu/decstation-ultrix/bin/
LDFLAGS = -T newscript -N
ASFLAGS = -mips1
CPPFLAGS = $(INCDIR)

# if you aren't cross-compiling:
# GCCDIR =
# LDFLAGS = -N -T 0
# ASFLAGS =
# CPPFLAGS = -P $(INCDIR)


CC = $(GCCDIR)gcc
AS = $(GCCDIR)as
LD = $(GCCDIR)ld
CPP = $(GCCDIR)cpp

INCDIR =-I../userprog -I../threads
CFLAGS = -G 0 -c $(INCDIR)

all: halt matmult sort testfiles acquirelock releaselock wait signal broadcast createlock createcondition destroylock destroycondition exec execfile

start.o: start.s ../userprog/syscall.h
	$(CPP) $(CPPFLAGS) start.s > strt.s
	$(AS) $(ASFLAGS) -o start.o strt.s
	rm strt.s

halt.o: halt.c
	$(CC) $(CFLAGS) -c halt.c
halt: halt.o start.o
	$(LD) $(LDFLAGS) start.o halt.o -o halt.coff
	../bin/coff2noff halt.coff halt

sort.o: sort.c
	$(CC) $(CFLAGS) -c sort.c
sort: sort.o start.o
	$(LD) $(LDFLAGS) start.o sort.o -o sort.coff
	../bin/coff2noff sort.coff sort

matmult.o: matmult.c
	$(CC) $(CFLAGS) -c matmult.c
matmult: matmult.o start.o
	$(LD) $(LDFLAGS) start.o matmult.o -o matmult.coff
	../bin/coff2noff matmult.coff matmult

testfiles.o: testfiles.c
	$(CC) $(CFLAGS) -c testfiles.c
testfiles: testfiles.o start.o
	$(LD) $(LDFLAGS) start.o testfiles.o -o testfiles.coff
	../bin/coff2noff testfiles.coff testfiles

acquirelock.o: acquirelock.c
	$(CC) $(CFLAGS) -c acquirelock.c
acquirelock: acquirelock.o start.o
	$(LD) $(LDFLAGS) start.o acquirelock.o -o acquirelock.coff
	../bin/coff2noff acquirelock.coff acquirelock

releaselock.o: releaselock.c
	$(CC) $(CFLAGS) -c releaselock.c
releaselock: releaselock.o start.o
	$(LD) $(LDFLAGS) start.o releaselock.o -o releaselock.coff
	../bin/coff2noff releaselock.coff releaselock

wait.o: wait.c
	$(CC) $(CFLAGS) -c wait.c
wait: wait.o start.o
	$(LD) $(LDFLAGS) start.o wait.o -o wait.coff
	../bin/coff2noff wait.coff wait

signal.o: signal.c
	$(CC) $(CFLAGS) -c signal.c
signal: signal.o start.o
	$(LD) $(LDFLAGS) start.o signal.o -o signal.coff
	../bin/coff2noff signal.coff signal

broadcast.o: broadcast.c
	$(CC) $(CFLAGS) -c broadcast.c
broadcast: broadcast.o start.o
	$(LD) $(LDFLAGS) start.o broadcast.o -o broadcast.coff
	../bin/coff2noff broadcast.coff broadcast

createlock.o: createlock.c
	$(CC) $(CFLAGS) -c createlock.c
createlock: createlock.o start.o
	$(LD) $(LDFLAGS) start.o createlock.o -o createlock.coff
	../bin/coff2noff createlock.coff createlock

createcondition.o: createcondition.c
	$(CC) $(CFLAGS) -c createcondition.c
createcondition: createcondition.o start.o
	$(LD) $(LDFLAGS) start.o createcondition.o -o createcondition.coff
	../bin/coff2noff createcondition.coff createcondition

destroylock.o: destroylock.c
	$(CC) $(CFLAGS) -c destroylock.c
destroylock: destroylock.o start.o
	$(LD) $(LDFLAGS) start.o destroylock.o -o destroylock.coff
	../bin/coff2noff destroylock.coff destroylock

destroycondition.o: destroycondition.c
	$(CC) $(CFLAGS) -c destroycondition.c
destroycondition: destroycondition.o start.o
	$(LD) $(LDFLAGS) start.o destroycondition.o -o destroycondition.coff
	../bin/coff2noff destroycondition.coff destroycondition

exec.o: exec.c
	$(CC) $(CFLAGS) -c exec.c
exec: exec.o start.o
	$(LD) $(LDFLAGS) start.o exec.o -o exec.coff
	../bin/coff2noff exec.coff exec

execfile.o: execfile.c
	$(CC) $(CFLAGS) -c execfile.c
execfile: execfile.o start.o
	$(LD) $(LDFLAGS) start.o execfile.o -o execfile.coff
	../bin/coff2noff execfile.coff execfile

clean:
	rm -f *.o *.coff
