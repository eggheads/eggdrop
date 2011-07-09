# Makefile for src/mod/server.mod/
# $Id: Makefile,v 1.18 2011/07/09 15:07:48 thommey Exp $

srcdir = .


doofus:
	@echo ""
	@echo "Let's try this from the right directory..."
	@echo ""
	@cd ../../../ && make

static: ../server.o

modules: ../../../server.$(MOD_EXT)

../server.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -DMAKING_MODS -c $(srcdir)/server.c
	@rm -f ../server.o
	mv server.o ../

../../../server.$(MOD_EXT): ../server.o
	$(LD) -o ../../../server.$(MOD_EXT) ../server.o $(XLIBS) $(MODULE_XLIBS)
	$(STRIP) ../../../server.$(MOD_EXT)

depend:
	$(CC) $(CFLAGS) -MM $(srcdir)/server.c -MT ../server.o > .depend

clean:
	@rm -f .depend *.o *.$(MOD_EXT) *~

distclean: clean

#safety hash
../server.o: .././server.mod/server.c ../../../src/mod/module.h \
 ../../../src/main.h ../../../config.h ../../../lush.h \
 ../../../src/lang.h ../../../src/eggdrop.h ../../../src/flags.h \
 ../../../src/cmdt.h ../../../src/tclegg.h ../../../src/tclhash.h \
 ../../../src/chan.h ../../../src/users.h ../../../src/compat/compat.h \
 ../../../src/compat/inet_aton.h ../../../src/compat/snprintf.h \
 ../../../src/compat/memset.h ../../../src/compat/memcpy.h \
 ../../../src/compat/strcasecmp.h ../../../src/compat/strdup.h \
 ../../../src/compat/strftime.h ../../../src/mod/modvals.h \
 ../../../src/tandem.h .././server.mod/server.h .././server.mod/servmsg.c \
 .././server.mod/cmdsserv.c .././server.mod/tclserv.c
