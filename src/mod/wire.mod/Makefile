# Makefile for src/mod/wire.mod/
# $Id: Makefile,v 1.12 2000/07/09 14:10:49 fabian Exp $

srcdir = .


doofus:
	@echo ""
	@echo "Let's try this from the right directory..."
	@echo ""
	@cd ../../../ && make

static: ../wire.o

modules: ../../../wire.$(MOD_EXT)

../wire.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -DMAKING_MODS -c $(srcdir)/wire.c
	@rm -f ../wire.o
	mv wire.o ../

../../../wire.$(MOD_EXT): ../wire.o
	$(LD) -o ../../../wire.$(MOD_EXT) ../wire.o
	$(STRIP) ../../../wire.$(MOD_EXT)

depend:
	$(CC) $(CFLAGS) $(CPPFLAGS) -MM $(srcdir)/wire.c > .depend

clean:
	@rm -f .depend *.o *.$(MOD_EXT) *~
distclean: clean

#safety hash
../wire.o: .././wire.mod/wire.c ../../../src/mod/module.h \
 ../../../src/main.h ../../../src/lang.h ../../../src/eggdrop.h \
 ../../../src/flags.h ../../../src/proto.h ../../../lush.h \
 ../../../src/misc_file.h ../../../src/cmdt.h ../../../src/tclegg.h \
 ../../../src/tclhash.h ../../../src/chan.h ../../../src/users.h \
 ../../../src/compat/compat.h ../../../src/compat/inet_aton.h \
 ../../../src/compat/snprintf.h ../../../src/compat/memset.h \
 ../../../src/compat/memcpy.h ../../../src/compat/strcasecmp.h \
 ../../../src/mod/modvals.h ../../../src/tandem.h ../wire.mod/wire.h
