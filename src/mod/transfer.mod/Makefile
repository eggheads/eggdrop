# Makefile for src/mod/transfer.mod/
# $Id: Makefile,v 1.6 2000/02/25 21:51:30 fabian Exp $

doofus:
	@echo ""
	@echo "Let's try this from the right directory..."
	@echo ""
	@cd ../../../; make

static: ../transfer.o

modules: ../../../transfer.so

../transfer.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -DMAKING_MODS -c transfer.c
	@rm -f ../transfer.o
	mv transfer.o ../

../../../transfer.so: ../transfer.o
	$(LD) -o ../../../transfer.so ../transfer.o
	$(STRIP) ../../../transfer.so

depend:
	$(CC) $(CFLAGS) $(CPPFLAGS) -MM *.c > .depend

clean:
	@rm -f .depend *.o *.so *~
distclean: clean

#safety hash
../transfer.o: transfer.c ../module.h ../../../config.h ../../main.h \
 ../../lang.h ../../eggdrop.h ../../flags.h ../../proto.h \
 ../../../lush.h ../../cmdt.h ../../tclegg.h ../../tclhash.h \
 ../../chan.h ../../users.h ../modvals.h ../../tandem.h transfer.h
