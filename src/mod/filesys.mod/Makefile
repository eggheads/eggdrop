# Makefile for src/mod/filesys.mod/
# $Id: Makefile,v 1.9 2000/02/01 20:36:18 fabian Exp $

doofus:
	@echo ""
	@echo "Let's try this from the right directory..."
	@echo ""
	@cd ../../../; make

static: ../filesys.o

modules: ../../../filesys.so

../filesys.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -DMAKING_MODS -c filesys.c
	rm -f ../filesys.o
	mv filesys.o ../

../../../filesys.so: ../filesys.o
	$(LD) -o ../../../filesys.so ../filesys.o
	$(STRIP) ../../../filesys.so

depend:
	$(CC) $(CFLAGS) $(CPPFLAGS) -MM *.c > .depend

clean:
	@rm -f .depend *.o *.so *~

#safety hash
../filesys.o: filesys.c ../../../config.h ../module.h ../../main.h \
 ../../lang.h ../../eggdrop.h ../../flags.h ../../proto.h \
 ../../../lush.h ../../cmdt.h ../../tclegg.h ../../tclhash.h \
 ../../chan.h ../../users.h ../modvals.h ../../tandem.h ../../stat.h \
 ../transfer.mod/transfer.h filesys.h files.h filedb3.c filedb3.h \
 files.c tclfiles.c dbcompat.c filelist.c filelist.h
