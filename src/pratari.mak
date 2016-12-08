# Small Prolog makefile for Mark Williams C version 3.0
# by Henri de Feraudy

CFLAGS = -VCSD -A -DATARI=1

OBJ =   prprint.o prscan.o prparse.o prunify.o pralloc.o\
	prbltin.o prlush.o prhash.o pratari.o prerror.o prcnsult.o\
	prdebug.o prassert.o 

sprolog: $(OBJ) prmain.o
	cc -VGEM -o sprolog.prg prmain.o $(OBJ) -lm 

sprolog.a : $(OBJ)
	ar ruv sprolog.a $(OBJ) ; ranlib sprolog.a; echo 'Thats it'
	
$(OBJ) :  prtypes.h 

prparse.o prscan.o : prlex.h

prlush.o : prlush.h

pp.o: pp.c
	cc -c -g pp.c

pp: pp.o
	cc -o $(HOME)/bin/pp -g pp.o

