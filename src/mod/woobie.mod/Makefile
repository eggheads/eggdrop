# Makefile for src/mod/woobie.mod/
# $Id: Makefile,v 1.10 1999/12/15 02:33:00 guppy Exp $

doofus:
	@echo ""
	@echo "Let's try this from the right directory..."
	@echo ""
	@cd ../../../; make

static: ../woobie.o

modules: ../../../woobie.so

../woobie.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -DMAKING_MODS -c woobie.c
	rm -f ../woobie.o
	mv woobie.o ../

../../../woobie.so: ../woobie.o
	$(LD) -o ../../../woobie.so ../woobie.o
	$(STRIP) ../../../woobie.so

depend:
	$(CC) $(CFLAGS) $(CPPFLAGS) -MM *.c > .depend

clean:
	@rm -f .depend *.o *.so *~

#safety hash
../woobie.o: woobie.c ../module.h ../../../config.h ../../main.h \
 ../../lang.h ../../eggdrop.h ../../flags.h ../../proto.h \
 ../../../lush.h ../../cmdt.h ../../tclegg.h ../../tclhash.h \
 ../../chan.h ../../users.h ../modvals.h ../../tandem.h
