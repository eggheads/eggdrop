# Makefile for src/mod/ctcp.mod/
# $Id: Makefile,v 1.10 1999/12/15 02:32:59 guppy Exp $

doofus:
	@echo ""
	@echo "Let's try this from the right directory..."
	@echo ""
	@cd ../../../; make

static: ../ctcp.o

modules: ../../../ctcp.so

../ctcp.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -DMAKING_MODS -c ctcp.c
	rm -f ../ctcp.o
	mv ctcp.o ../

../../../ctcp.so: ../ctcp.o
	$(LD) -o ../../../ctcp.so ../ctcp.o
	$(STRIP) ../../../ctcp.so

depend:
	$(CC) $(CFLAGS) $(CPPFLAGS) -MM *.c > .depend

clean:
	@rm -f .depend *.o *.so *~

#safety hash
../ctcp.o: ctcp.c ctcp.h ../module.h ../../../config.h ../../main.h \
 ../../lang.h ../../eggdrop.h ../../flags.h ../../proto.h \
 ../../../lush.h ../../cmdt.h ../../tclegg.h ../../tclhash.h \
 ../../chan.h ../../users.h ../modvals.h ../../tandem.h \
 ../server.mod/server.h
