# Makefile for src/mod/share.mod/
# $Id: Makefile,v 1.10 1999/12/15 02:33:00 guppy Exp $

doofus:
	@echo ""
	@echo "Let's try this from the right directory..."
	@echo ""
	@cd ../../../; make

static: ../share.o

modules: ../../../share.so

../share.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -DMAKING_MODS -c share.c
	rm -f ../share.o
	mv share.o ../

../../../share.so: ../share.o
	$(LD) -o ../../../share.so ../share.o
	$(STRIP) ../../../share.so

depend:
	$(CC) $(CFLAGS) $(CPPFLAGS) -MM *.c > .depend

clean:
	@rm -f .depend *.o *.so *~

#safety hash
../share.o: share.c ../module.h ../../../config.h ../../main.h \
 ../../lang.h ../../eggdrop.h ../../flags.h ../../proto.h \
 ../../../lush.h ../../cmdt.h ../../tclegg.h ../../tclhash.h \
 ../../chan.h ../../users.h ../modvals.h ../../tandem.h \
 ../transfer.mod/transfer.h ../channels.mod/channels.h
