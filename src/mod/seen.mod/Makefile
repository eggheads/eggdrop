# Makefile for src/mod/seen.mod/

srcdir = .


doofus:
	@echo "" && \
	echo "Let's try this from the right directory..." && \
	echo "" && \
	cd ../../../ && $(MAKE)

static: ../seen.o

modules: ../../../seen.$(MOD_EXT)

../seen.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -DMAKING_MODS -c $(srcdir)/seen.c && mv -f seen.o ../

../../../seen.$(MOD_EXT): ../seen.o
	$(LD) $(CFLAGS) -o ../../../seen.$(MOD_EXT) ../seen.o $(XLIBS) $(MODULE_XLIBS) && $(STRIP) ../../../seen.$(MOD_EXT)

depend:
	$(CC) $(CFLAGS) -MM $(srcdir)/seen.c -MT ../seen.o > .depend

clean:
	@rm -f .depend *.o *.$(MOD_EXT) *~

distclean: clean

#safety hash
../seen.o: .././seen.mod/seen.c ../../../src/mod/module.h \
 ../../../src/main.h ../../../config.h ../../../eggint.h ../../../lush.h \
 ../../../src/lang.h ../../../src/eggdrop.h ../../../src/compat/in6.h \
 ../../../src/flags.h ../../../src/cmdt.h ../../../src/tclegg.h \
 ../../../src/tclhash.h ../../../src/chan.h ../../../src/users.h \
 ../../../src/compat/compat.h ../../../src/compat/base64.h \
 ../../../src/compat/inet_aton.h ../../../src/compat/snprintf.h \
 ../../../src/compat/explicit_bzero.h ../../../src/compat/strlcpy.h \
 ../../../src/mod/modvals.h ../../../src/tandem.h ../../../src/users.h \
 ../../../src/chan.h ../../../src/mod/channels.mod/channels.h
