# Makefile for src/mod/notes.mod
# $Id: Makefile,v 1.18 2011/07/09 15:07:48 thommey Exp $

srcdir = .


doofus:
	@echo ""
	@echo "Let's try this from the right directory..."
	@echo ""
	@cd ../../../ && make

static: ../notes.o

modules: ../../../notes.$(MOD_EXT)

../notes.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -DMAKING_MODS -c $(srcdir)/notes.c
	@rm -f ../notes.o
	mv notes.o ../

../../../notes.$(MOD_EXT): ../notes.o
	$(LD) -o ../../../notes.$(MOD_EXT) ../notes.o $(XLIBS) $(MODULE_XLIBS)
	$(STRIP) ../../../notes.$(MOD_EXT)

depend:
	$(CC) $(CFLAGS) -MM $(srcdir)/notes.c -MT ../notes.o > .depend

clean:
	@rm -f .depend *.o *.$(MOD_EXT) *~

distclean: clean

#safety hash
../notes.o: .././notes.mod/notes.c ../../../src/mod/module.h \
 ../../../src/main.h ../../../config.h ../../../lush.h \
 ../../../src/lang.h ../../../src/eggdrop.h ../../../src/flags.h \
 ../../../src/cmdt.h ../../../src/tclegg.h ../../../src/tclhash.h \
 ../../../src/chan.h ../../../src/users.h ../../../src/compat/compat.h \
 ../../../src/compat/inet_aton.h ../../../src/compat/snprintf.h \
 ../../../src/compat/memset.h ../../../src/compat/memcpy.h \
 ../../../src/compat/strcasecmp.h ../../../src/compat/strdup.h \
 ../../../src/compat/strftime.h ../../../src/mod/modvals.h \
 ../../../src/tandem.h .././notes.mod/notes.h .././notes.mod/cmdsnote.c
