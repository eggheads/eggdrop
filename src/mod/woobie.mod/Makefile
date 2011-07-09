# Makefile for src/mod/woobie.mod/
# $Id: Makefile,v 1.15 2011/07/09 15:07:48 thommey Exp $

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
	$(LD) -o ../../../woobie.$(MOD_EXT) ../woobie.o $(XLIBS) $(MODULE_XLIBS)
	$(STRIP) ../../../woobie.$(MOD_EXT)

depend:
	$(CC) $(CFLAGS) -MM $(srcdir)/woobie.c -MT ../woobie.o > .depend

clean:
	@rm -f .depend *.o *.$(MOD_EXT) *~

distclean: clean

#safety hash
../woobie.o: woobie.c ../../../src/mod/module.h ../../../src/main.h \
 ../../../config.h ../../../lush.h ../../../src/lang.h \
 ../../../src/eggdrop.h ../../../src/flags.h ../../../src/cmdt.h \
 ../../../src/tclegg.h ../../../src/tclhash.h ../../../src/chan.h \
 ../../../src/users.h ../../../src/compat/compat.h \
 ../../../src/compat/inet_aton.h ../../../src/compat/snprintf.h \
 ../../../src/compat/memset.h ../../../src/compat/memcpy.h \
 ../../../src/compat/strcasecmp.h ../../../src/compat/strdup.h \
 ../../../src/compat/strftime.h ../../../src/mod/modvals.h \
 ../../../src/tandem.h
