# Makefile for src/mod/blowfish.mod/
# $Id: Makefile,v 1.17 2011/07/09 15:07:48 thommey Exp $

srcdir = .


doofus:
	@echo ""
	@echo "Let's try this from the right directory..."
	@echo ""
	@cd ../../../ && make

static: ../blowfish.o

modules: ../../../blowfish.$(MOD_EXT)

../blowfish.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -DMAKING_MODS -c $(srcdir)/blowfish.c
	@rm -f ../blowfish.o
	mv blowfish.o ../

../../../blowfish.$(MOD_EXT): ../blowfish.o
	$(LD) -o ../../../blowfish.$(MOD_EXT) ../blowfish.o $(XLIBS) $(MODULE_XLIBS)
	$(STRIP) ../../../blowfish.$(MOD_EXT)

depend:
	$(CC) $(CFLAGS) -MM $(srcdir)/blowfish.c -MT ../blowfish.o > .depend

clean:
	@rm -f .depend *.o *.$(MOD_EXT) *~

distclean: clean

#safety hash
../blowfish.o: .././blowfish.mod/blowfish.c ../../../src/mod/module.h \
 ../../../src/main.h ../../../config.h ../../../lush.h \
 ../../../src/lang.h ../../../src/eggdrop.h ../../../src/flags.h \
 ../../../src/cmdt.h ../../../src/tclegg.h ../../../src/tclhash.h \
 ../../../src/chan.h ../../../src/users.h ../../../src/compat/compat.h \
 ../../../src/compat/inet_aton.h ../../../src/compat/snprintf.h \
 ../../../src/compat/memset.h ../../../src/compat/memcpy.h \
 ../../../src/compat/strcasecmp.h ../../../src/compat/strdup.h \
 ../../../src/compat/strftime.h ../../../src/mod/modvals.h \
 ../../../src/tandem.h .././blowfish.mod/blowfish.h \
 .././blowfish.mod/bf_tab.h
