# Makefile for src/mod/seen.mod/
# $Id: Makefile,v 1.17 2011/07/09 15:07:48 thommey Exp $

srcdir = .


doofus:
	@echo ""
	@echo "Let's try this from the right directory..."
	@echo ""
	@cd ../../../ && make

static: ../seen.o

modules: ../../../seen.$(MOD_EXT)

../seen.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -DMAKING_MODS -c $(srcdir)/seen.c
	@rm -f ../seen.o
	mv seen.o ../

../../../seen.$(MOD_EXT): ../seen.o
	$(LD) -o ../../../seen.$(MOD_EXT) ../seen.o $(XLIBS) $(MODULE_XLIBS)
	$(STRIP) ../../../seen.$(MOD_EXT)

depend:
	$(CC) $(CFLAGS) -MM $(srcdir)/seen.c -MT ../seen.o > .depend

clean:
	@rm -f .depend *.o *.$(MOD_EXT) *~

distclean: clean

#safety hash
../seen.o: .././seen.mod/seen.c ../../../src/mod/module.h \
 ../../../src/main.h ../../../config.h ../../../lush.h \
 ../../../src/lang.h ../../../src/eggdrop.h ../../../src/flags.h \
 ../../../src/cmdt.h ../../../src/tclegg.h ../../../src/tclhash.h \
 ../../../src/chan.h ../../../src/users.h ../../../src/compat/compat.h \
 ../../../src/compat/inet_aton.h ../../../src/compat/snprintf.h \
 ../../../src/compat/memset.h ../../../src/compat/memcpy.h \
 ../../../src/compat/strcasecmp.h ../../../src/compat/strdup.h \
 ../../../src/compat/strftime.h ../../../src/mod/modvals.h \
 ../../../src/tandem.h ../../../src/users.h ../../../src/chan.h \
 ../../../src/mod/channels.mod/channels.h
