# Makefile for src/mod/ctcp.mod/
# $Id: Makefile,v 1.17 2011/07/09 15:07:48 thommey Exp $

srcdir = .


doofus:
	@echo ""
	@echo "Let's try this from the right directory..."
	@echo ""
	@cd ../../../ && make

static: ../ctcp.o

modules: ../../../ctcp.$(MOD_EXT)

../ctcp.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -DMAKING_MODS -c $(srcdir)/ctcp.c
	@rm -f ../ctcp.o
	mv ctcp.o ../

../../../ctcp.$(MOD_EXT): ../ctcp.o
	$(LD) -o ../../../ctcp.$(MOD_EXT) ../ctcp.o $(XLIBS) $(MODULE_XLIBS)
	$(STRIP) ../../../ctcp.$(MOD_EXT)

depend:
	$(CC) $(CFLAGS) -MM $(srcdir)/ctcp.c -MT ../ctcp.o > .depend

clean:
	@rm -f .depend *.o *.$(MOD_EXT) *~

distclean: clean

#safety hash
../ctcp.o: .././ctcp.mod/ctcp.c .././ctcp.mod/ctcp.h \
 ../../../src/mod/module.h ../../../src/main.h ../../../config.h \
 ../../../lush.h ../../../src/lang.h ../../../src/eggdrop.h \
 ../../../src/flags.h ../../../src/cmdt.h ../../../src/tclegg.h \
 ../../../src/tclhash.h ../../../src/chan.h ../../../src/users.h \
 ../../../src/compat/compat.h ../../../src/compat/inet_aton.h \
 ../../../src/compat/snprintf.h ../../../src/compat/memset.h \
 ../../../src/compat/memcpy.h ../../../src/compat/strcasecmp.h \
 ../../../src/compat/strdup.h ../../../src/compat/strftime.h \
 ../../../src/mod/modvals.h ../../../src/tandem.h \
 ../../../src/mod/server.mod/server.h
