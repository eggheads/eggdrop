# Makefile for src/mod/console.mod/

srcdir = .


doofus:
	@echo "" && \
	echo "Let's try this from the right directory..." && \
	echo "" && \
	cd ../../../ && $(MAKE)

static: ../console.o

modules: ../../../console.$(MOD_EXT)

../console.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -DMAKING_MODS -c $(srcdir)/console.c && mv -f console.o ../

../../../console.$(MOD_EXT): ../console.o
	$(LD) $(CFLAGS) -o ../../../console.$(MOD_EXT) ../console.o $(XLIBS) $(MODULE_XLIBS) && $(STRIP) ../../../console.$(MOD_EXT)

depend:
	$(CC) $(CFLAGS) -MM $(srcdir)/console.c -MT ../console.o > .depend

clean:
	@rm -f .depend *.o *.$(MOD_EXT) *~

distclean: clean

#safety hash
../console.o: .././console.mod/console.c ../../../src/mod/module.h \
 ../../../src/main.h ../../../config.h ../../../eggint.h ../../../lush.h \
 ../../../src/lang.h ../../../src/eggdrop.h ../../../src/compat/in6.h \
 ../../../src/flags.h ../../../src/cmdt.h ../../../src/tclegg.h \
 ../../../src/tclhash.h ../../../src/chan.h ../../../src/users.h \
 ../../../src/compat/compat.h ../../../src/compat/base64.h \
 ../../../src/compat/inet_aton.h ../../../src/compat/snprintf.h \
 ../../../src/compat/explicit_bzero.h ../../../src/compat/strlcpy.h \
 ../../../src/mod/modvals.h ../../../src/tandem.h \
 .././console.mod/console.h
