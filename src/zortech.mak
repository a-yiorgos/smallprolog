# makefile for Small Prolog on PC with Zortech C
# note that the compact memory model is used
# you may need to change to the large model this if you make your own code.

.c.obj:
	ztc -c -mc -f -DMSDOS=1 $<

SRC = pralloc.c prassert.c prbltin.c prcnsult.c prdebug.c prerror.c \
 prhash.c prlush.c prmain.c prparse.c prprint.c prscan.c pribmpc.c prunify.c

OBJ = pralloc.obj prassert.obj prbltin.obj prcnsult.obj prdebug.obj\
	prerror.obj  prhash.obj prlush.obj prparse.obj \
	prprint.obj prscan.obj pribmpc.obj prunify.obj  prmain.obj

LINK1 = pralloc.obj+prassert.obj+prbltin.obj+prcnsult.obj+prdebug.obj+
LINK2 =	prerror.obj+prhash.obj+prlush.obj+prparse.obj+
LINK3 =	prprint.obj+prscan.obj+pribmpc.obj+prunify.obj+prmain.obj

sprolog.exe : $(OBJ)
	echo $(LINK1) >temp
	echo $(LINK2) >>temp
	echo $(LINK3) >>temp
	echo sprolog.exe >>temp
	blink @temp
	del temp

pp.exe : pp.c
	ztc pp.c 
	
prlush.obj: prtypes.h prolog.h prlush.h
prscan.obj: prtypes.h prolog.h prlex.h
prbltin.obj: prbltin.h prtypes.h prolog.h
pralloc.obj: prtypes.h prolog.h 
prassert.obj : prtypes.h prolog.h 
prcnsult.obj : prtypes.h prolog.h 
prdebug.obj: prtypes.h prolog.h 
prerror.obj  : prtypes.h prolog.h 
prhash.obj : prtypes.h prolog.h 
prparse.obj : prtypes.h prolog.h 
prprint.obj : prtypes.h prolog.h prlex.h
pribmpc.obj : prtypes.h prolog.h 
prunify.obj : prtypes.h prolog.h 

	

