# Makefile for src/mod/blowfish.mod/
# $Id: Makefile,v 1.10 1999/12/15 02:32:59 guppy Exp $

doofus:
	@echo ""
	@echo "Let's try this from the right directory..."
	@echo ""
	@cd ../../../; make

static: ../blowfish.o

modules: ../../../blowfish.so

../blowfish.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -DMAKING_MODS -c blowfish.c
	rm -f ../blowfish.o
	mv blowfish.o ../

../../../blowfish.so: ../blowfish.o
	$(LD) -o ../../../blowfish.so ../blowfish.o
	$(STRIP) ../../../blowfish.so

depend:
	$(CC) $(CFLAGS) $(CPPFLAGS) -MM *.c > .depend

clean:
	@rm -f .depend *.o *.so *~

#safety hash
../blowfish.o: blowfish.c ../module.h ../../../config.h ../../main.h \
 ../../lang.h ../../eggdrop.h ../../flags.h ../../proto.h \
 ../../../lush.h ../../cmdt.h ../../tclegg.h ../../tclhash.h \
 ../../chan.h ../../users.h ../modvals.h ../../tandem.h blowfish.h \
 bf_tab.h
