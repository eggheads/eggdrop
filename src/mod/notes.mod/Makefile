# Makefile for src/mod/notes.mod
# $Id: Makefile,v 1.6 1999/12/21 17:35:30 fabian Exp $

doofus:
	@echo ""
	@echo "Let's try this from the right directory..."
	@echo ""
	@cd ../../../; make

static: ../notes.o

modules: ../../../notes.so

../notes.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -DMAKING_MODS -c notes.c
	rm -f ../notes.o
	mv notes.o ../

../../../notes.so: ../notes.o
	$(LD) -o ../../../notes.so ../notes.o
	$(STRIP) ../../../notes.so

depend:
	$(CC) $(CFLAGS) $(CPPFLAGS) -MM *.c > .depend

clean:
	@rm -f .depend *.o *.so *~

#safety hash
../notes.o: notes.c ../module.h ../../../config.h ../../main.h \
 ../../lang.h ../../eggdrop.h ../../flags.h ../../proto.h \
 ../../../lush.h ../../cmdt.h ../../tclegg.h ../../tclhash.h \
 ../../chan.h ../../users.h ../modvals.h ../../tandem.h notes.h \
 cmdsnote.c
