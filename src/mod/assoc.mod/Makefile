# Makefile for src/mod/assoc.mod/
# $Id: Makefile,v 1.10 1999/12/15 02:32:58 guppy Exp $

doofus:
	@echo ""
	@echo "Let's try this from the right directory..."
	@echo ""
	@cd ../../../; make

static: ../assoc.o

modules: ../../../assoc.so

../assoc.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -DMAKING_MODS -c assoc.c
	rm -f ../assoc.o
	mv assoc.o ../

../../../assoc.so: ../assoc.o
	$(LD) -o ../../../assoc.so ../assoc.o
	$(STRIP) ../../../assoc.so

depend:
	$(CC) $(CFLAGS) $(CPPFLAGS) -MM *.c > .depend

clean:
	@rm -f .depend *.o *.so *~

#safety hash
../assoc.o: assoc.c ../module.h ../../../config.h ../../main.h \
 ../../lang.h ../../eggdrop.h ../../flags.h ../../proto.h \
 ../../../lush.h ../../cmdt.h ../../tclegg.h ../../tclhash.h \
 ../../chan.h ../../users.h ../modvals.h ../../tandem.h
