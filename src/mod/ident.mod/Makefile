# Makefile for src/mod/ident.mod/

srcdir = .


doofus:
	@echo "" && \
	echo "Let's try this from the right directory..." && \
	echo "" && \
	cd ../../../ && $(MAKE)

static: ../ident.o

modules: ../../../ident.$(MOD_EXT)

../ident.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -DMAKING_MODS -c $(srcdir)/ident.c && mv -f ident.o ../

../../../ident.$(MOD_EXT): ../ident.o
	$(LD) $(CFLAGS) -o ../../../ident.$(MOD_EXT) ../ident.o $(XLIBS) $(MODULE_XLIBS) && $(STRIP) ../../../ident.$(MOD_EXT)

depend:
	$(CC) $(CFLAGS) -MM $(srcdir)/ident.c -MT ../ident.o > .depend

clean:
	@rm -f .depend *.o *.$(MOD_EXT) *~

distclean: clean

#safety hash
../ident.o: .././ident.mod/ident.c ../../../src/mod/module.h \
 ../../../src/main.h ../../../config.h ../../../eggint.h ../../../lush.h \
 ../../../src/lang.h ../../../src/eggdrop.h ../../../src/compat/in6.h \
 ../../../src/flags.h ../../../src/cmdt.h ../../../src/tclegg.h \
 ../../../src/tclhash.h ../../../src/chan.h ../../../src/users.h \
 ../../../src/compat/compat.h ../../../src/compat/base64.h \
 ../../../src/compat/inet_aton.h ../../../src/compat/snprintf.h \
 ../../../src/compat/explicit_bzero.h ../../../src/compat/strlcpy.h \
 ../../../src/mod/modvals.h ../../../src/tandem.h \
 ../../../src/mod/server.mod/server.h
