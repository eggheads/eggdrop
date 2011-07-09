# Makefile for src/mod/transfer.mod/
# $Id: Makefile,v 1.18 2011/07/09 15:07:48 thommey Exp $

srcdir = .


doofus:
	@echo ""
	@echo "Let's try this from the right directory..."
	@echo ""
	@cd ../../../ && make

static: ../transfer.o

modules: ../../../transfer.$(MOD_EXT)

../transfer.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -DMAKING_MODS -c $(srcdir)/transfer.c
	@rm -f ../transfer.o
	mv transfer.o ../

../../../transfer.$(MOD_EXT): ../transfer.o
	$(LD) -o ../../../transfer.$(MOD_EXT) ../transfer.o $(XLIBS) $(MODULE_XLIBS)
	$(STRIP) ../../../transfer.$(MOD_EXT)

depend:
	$(CC) $(CFLAGS) -MM $(srcdir)/transfer.c -MT ../transfer.o > .depend

clean:
	@rm -f .depend *.o *.$(MOD_EXT) *~

distclean: clean

#safety hash
../transfer.o: .././transfer.mod/transfer.c ../../../src/mod/module.h \
 ../../../src/main.h ../../../config.h ../../../lush.h \
 ../../../src/lang.h ../../../src/eggdrop.h ../../../src/flags.h \
 ../../../src/cmdt.h ../../../src/tclegg.h ../../../src/tclhash.h \
 ../../../src/chan.h ../../../src/users.h ../../../src/compat/compat.h \
 ../../../src/compat/inet_aton.h ../../../src/compat/snprintf.h \
 ../../../src/compat/memset.h ../../../src/compat/memcpy.h \
 ../../../src/compat/strcasecmp.h ../../../src/compat/strdup.h \
 ../../../src/compat/strftime.h ../../../src/mod/modvals.h \
 ../../../src/tandem.h ../../../src/users.h .././transfer.mod/transfer.h \
 .././transfer.mod/transferfstat.c .././transfer.mod/transferqueue.c \
 .././transfer.mod/tcltransfer.c
