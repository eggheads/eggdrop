# Makefile for src/mod/console.mod/
# $Id: Makefile,v 1.10 1999/12/15 02:32:59 guppy Exp $

doofus:
	@echo ""
	@echo "Let's try this from the right directory..."
	@echo ""
	@cd ../../../; make

static: ../console.o

modules: ../../../console.so

../console.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -DMAKING_MODS -c console.c
	rm -f ../console.o
	mv console.o ../

../../../console.so: ../console.o
	$(LD) -o ../../../console.so ../console.o
	$(STRIP) ../../../console.so

depend:
	$(CC) $(CFLAGS) $(CPPFLAGS) -MM *.c > .depend

clean:
	@rm -f .depend *.o *.so *~

#safety hash
../console.o: console.c ../module.h ../../../config.h ../../main.h \
 ../../lang.h ../../eggdrop.h ../../flags.h ../../proto.h \
 ../../../lush.h ../../cmdt.h ../../tclegg.h ../../tclhash.h \
 ../../chan.h ../../users.h ../modvals.h ../../tandem.h
