#
# Program: sprolog
#

.c.obj:
	qcl -c  -W1 -Ze -Zr -AL $*.c

pralloc.obj : pralloc.c

prassert.obj : prassert.c

prbltin.obj : prbltin.c

prcnsult.obj : prcnsult.c

prdebug.obj : prdebug.c

prerror.obj : prerror.c

prhash.obj : prhash.c

pribmpc.obj : pribmpc.c

prlush.obj : prlush.c

prmain.obj : prmain.c

prparse.obj : prparse.c

prprint.obj : prprint.c

prscan.obj : prscan.c

prunify.obj : prunify.c

sprolog.exe : pralloc.obj prassert.obj prbltin.obj prcnsult.obj prdebug.obj  \
		prerror.obj prhash.obj pribmpc.obj prlush.obj prmain.obj  \
		prparse.obj prprint.obj prscan.obj prunify.obj 
	del Quickc.lnk
	echo pralloc.obj+ >>Quickc.lnk
	echo prassert.obj+ >>Quickc.lnk
	echo prbltin.obj+ >>Quickc.lnk
	echo prcnsult.obj+ >>Quickc.lnk
	echo prdebug.obj+ >>Quickc.lnk
	echo prerror.obj+ >>Quickc.lnk
	echo prhash.obj+ >>Quickc.lnk
	echo pribmpc.obj+ >>Quickc.lnk
	echo prlush.obj+ >>Quickc.lnk
	echo prmain.obj+ >>Quickc.lnk
	echo prparse.obj+ >>Quickc.lnk
	echo prprint.obj+ >>Quickc.lnk
	echo prscan.obj+ >>Quickc.lnk
	echo prunify.obj  >>Quickc.lnk
	echo sprolog.exe >>Quickc.lnk
	echo sprolog.map >>Quickc.lnk
	link @Quickc.lnk /NOI $(LDFLAGS);
