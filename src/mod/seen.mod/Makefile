# Makefile for src/mod/seen.mod/
# $Id: Makefile,v 1.6 2000/02/25 21:51:30 fabian Exp $

doofus:
	@echo ""
	@echo "Let's try this from the right directory..."
	@echo ""
	@cd ../../../; make

static: ../seen.o

modules: ../../../seen.so

../seen.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -DMAKING_MODS -c seen.c
	@rm -f ../seen.o
	mv seen.o ../

../../../seen.so: ../seen.o
	$(LD) -o ../../../seen.so ../seen.o
	$(STRIP) ../../../seen.so

depend:
	$(CC) $(CFLAGS) $(CPPFLAGS) -MM *.c > .depend

clean:
	@rm -f .depend *.o *.so *~
distclean: clean

#safety hash
../seen.o: seen.c ../module.h ../../../config.h ../../main.h ../../lang.h \
 ../../eggdrop.h ../../flags.h ../../proto.h ../../../lush.h \
 ../../cmdt.h ../../tclegg.h ../../tclhash.h ../../chan.h \
 ../../users.h ../modvals.h ../../tandem.h ../channels.mod/channels.h
