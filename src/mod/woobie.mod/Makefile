# Makefile for src/mod/woobie.mod/
# $Id: Makefile,v 1.8 2000/03/31 23:04:22 fabian Exp $

srcdir = .


doofus:
	@echo ""
	@echo "Let's try this from the right directory..."
	@echo ""
	@cd ../../../ && make

static: ../woobie.o

modules: ../../../woobie.so

../woobie.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -DMAKING_MODS -c $(srcdir)/woobie.c
	@rm -f ../woobie.o
	mv woobie.o ../

../../../woobie.so: ../woobie.o
	$(LD) -o ../../../woobie.so ../woobie.o
	$(STRIP) ../../../woobie.so

depend:
	$(CC) $(CFLAGS) $(CPPFLAGS) -MM $(srcdir)/woobie.c > .depend

clean:
	@rm -f .depend *.o *.so *~
distclean: clean

#safety hash
../woobie.o: .././woobie.mod/woobie.c ../../../src/mod/module.h \
 ../../../src/main.h ../../../src/lang.h ../../../src/eggdrop.h \
 ../../../src/flags.h ../../../src/proto.h ../../../lush.h \
 ../../../src/misc_file.h ../../../src/cmdt.h ../../../src/tclegg.h \
 ../../../src/tclhash.h ../../../src/chan.h ../../../src/users.h \
 ../../../src/compat/compat.h ../../../src/compat/inet_aton.h \
 ../../../src/compat/snprintf.h ../../../src/compat/memset.h \
 ../../../src/compat/memcpy.h ../../../src/compat/strcasecmp.h \
 ../../../src/mod/modvals.h ../../../src/tandem.h
