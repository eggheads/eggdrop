# Makefile for src/mod/dns.mod/
# $Id: Makefile,v 1.5 2000/01/02 02:42:11 fabian Exp $

doofus:
	@echo ""
	@echo "Let's try this from the right directory..."
	@echo ""
	@cd ../../../; make

static: ../dns.o

modules: ../../../dns.so

../dns.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -DMAKING_MODS -c dns.c
	rm -f ../dns.o
	mv dns.o ../

../../../dns.so: ../dns.o
	$(LD) -o ../../../dns.so ../dns.o $(RESLIB)
	$(STRIP) ../../../dns.so

depend:
	$(CC) $(CFLAGS) $(CPPFLAGS) -MM *.c > .depend

clean:
	@rm -f .depend *.o *.so *~

#safety hash
../dns.o: dns.c ../module.h ../../../config.h ../../main.h ../../lang.h \
 ../../eggdrop.h ../../flags.h ../../proto.h ../../../lush.h \
 ../../cmdt.h ../../tclegg.h ../../tclhash.h ../../chan.h ../../users.h \
 ../modvals.h ../../tandem.h dns.h coredns.c
