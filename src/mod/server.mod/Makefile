# Makefile for src/mod/server.mod/
# $Id: Makefile,v 1.11 1999/12/15 02:33:00 guppy Exp $

doofus:
	@echo ""
	@echo "Let's try this from the right directory..."
	@echo ""
	@cd ../../../; make

static: ../server.o

modules: ../../../server.so

../server.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -DMAKING_MODS -c server.c
	rm -f ../server.o
	mv server.o ../

../../../server.so: ../server.o
	$(LD) -o ../../../server.so ../server.o
	$(STRIP) ../../../server.so

depend:
	$(CC) $(CFLAGS) $(CPPFLAGS) -MM *.c > .depend

clean:
	@rm -f .depend *.o *.so *~

#safety hash
../server.o: server.c ../module.h ../../../config.h ../../main.h \
 ../../lang.h ../../eggdrop.h ../../flags.h ../../proto.h \
 ../../../lush.h ../../cmdt.h ../../tclegg.h ../../tclhash.h \
 ../../chan.h ../../users.h ../modvals.h ../../tandem.h server.h \
 servmsg.c cmdsserv.c tclserv.c
