# Small Prolog makefile for SUN-unix
# by Henri de Feraudy

CFLAGS=-DSUN -g

OBJ =   prprint.o prscan.o prparse.o prunify.o pralloc.o\
	prbltin.o prlush.o prhash.o prsun.o prerror.o prcnsult.o\
	prdebug.o prassert.o


sprolog: $(OBJ) prmain.o
	$(CC) -o sprolog -g prmain.o $(OBJ) 

sprolog.a : $(OBJ)
	ar ruv sprolog.a $(OBJ) ; ranlib sprolog.a; echo 'Thats it'
	
$(OBJ) :  prtypes.h 

prparse.o prscan.o : prlex.h

prlush.o : prlush.h

pp.o: pp.c
	cc -c -g pp.c

pp: pp.o
	cc -o pp -g pp.o

