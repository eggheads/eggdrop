# Makefile for src/mod/console.mod/
# $Id: Makefile,v 1.20 2011/07/09 15:07:48 thommey Exp $

srcdir = .


doofus:
	@echo ""
	@echo "Let's try this from the right directory..."
	@echo ""
	@cd ../../../ && make

static: ../console.o

modules: ../../../console.$(MOD_EXT)

../console.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -DMAKING_MODS -c $(srcdir)/console.c
	@rm -f ../console.o
	mv console.o ../

../../../console.$(MOD_EXT): ../console.o
	$(LD) -o ../../../console.$(MOD_EXT) ../console.o $(XLIBS) $(MODULE_XLIBS)
	$(STRIP) ../../../console.$(MOD_EXT)

depend:
	$(CC) $(CFLAGS) -MM $(srcdir)/console.c -MT ../console.o > .depend

clean:
	@rm -f .depend *.o *.$(MOD_EXT) *~

distclean: clean

#safety hash
../console.o: .././console.mod/console.c ../../../src/mod/module.h \
 ../../../src/main.h ../../../config.h ../../../lush.h \
 ../../../src/lang.h ../../../src/eggdrop.h ../../../src/flags.h \
 ../../../src/cmdt.h ../../../src/tclegg.h ../../../src/tclhash.h \
 ../../../src/chan.h ../../../src/users.h ../../../src/compat/compat.h \
 ../../../src/compat/inet_aton.h ../../../src/compat/snprintf.h \
 ../../../src/compat/memset.h ../../../src/compat/memcpy.h \
 ../../../src/compat/strcasecmp.h ../../../src/compat/strdup.h \
 ../../../src/compat/strftime.h ../../../src/mod/modvals.h \
 ../../../src/tandem.h .././console.mod/console.h
