# Makefile for src/mod/woobie.mod/
# $Id: Makefile,v 1.10 2003/04/01 05:33:41 wcc Exp $

srcdir = .


doofus:
	@echo ""
	@echo "Let's try this from the right directory..."
	@echo ""
	@cd ../../../ && make

static: ../woobie.o

modules: ../../../woobie.$(MOD_EXT)

../woobie.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -DMAKING_MODS -c $(srcdir)/woobie.c
	@rm -f ../woobie.o
	mv woobie.o ../

../../../woobie.$(MOD_EXT): ../woobie.o
	$(LD) -o ../../../woobie.$(MOD_EXT) ../woobie.o
	$(STRIP) ../../../woobie.$(MOD_EXT)

depend:
	$(CC) $(CFLAGS) $(CPPFLAGS) -MM $(srcdir)/woobie.c > .depend

clean:
	@rm -f .depend *.o *.$(MOD_EXT) *~
distclean: clean

#safety hash
../woobie.o: .././woobie.mod/woobie.c ../../../src/mod/module.h \
 ../../../src/main.h ../../../src/lang.h ../../../src/eggdrop.h \
 ../../../src/flags.h ../../../src/proto.h ../../../lush.h \
 ../../../src/misc_file.h ../../../src/cmdt.h ../../../src/tclegg.h \
 ../../../src/tclhash.h ../../../src/chan.h ../../../src/users.h \
 ../../../src/compat/compat.h ../../../src/compat/inet_aton.h \
 ../../../src/compat/inet_ntop.h ../../../src/compat/snprintf.h \
 ../../../src/compat/memset.h ../../../src/compat/memcpy.h \
 ../../../src/compat/strcasecmp.h ../../../src/mod/modvals.h \
 ../../../src/tandem.h
