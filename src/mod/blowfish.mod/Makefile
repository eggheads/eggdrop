# Makefile for src/mod/blowfish.mod/

srcdir = .


doofus:
	@echo "" && \
	echo "Let's try this from the right directory..." && \
	echo "" && \
	cd ../../../ && $(MAKE)

static: ../blowfish.o

modules: ../../../blowfish.$(MOD_EXT)

../blowfish.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -DMAKING_MODS -c $(srcdir)/blowfish.c && mv -f blowfish.o ../

../../../blowfish.$(MOD_EXT): ../blowfish.o
	$(LD) $(CFLAGS) -o ../../../blowfish.$(MOD_EXT) ../blowfish.o $(XLIBS) $(MODULE_XLIBS) && $(STRIP) ../../../blowfish.$(MOD_EXT)

depend:
	$(CC) $(CFLAGS) -MM $(srcdir)/blowfish.c -MT ../blowfish.o > .depend

clean:
	@rm -f .depend *.o *.$(MOD_EXT) *~

distclean: clean

#safety hash
../blowfish.o: .././blowfish.mod/blowfish.c ../../../src/mod/module.h \
 ../../../src/main.h ../../../config.h ../../../eggint.h ../../../lush.h \
 ../../../src/lang.h ../../../src/eggdrop.h ../../../src/compat/in6.h \
 ../../../src/flags.h ../../../src/cmdt.h ../../../src/tclegg.h \
 ../../../src/tclhash.h ../../../src/chan.h ../../../src/users.h \
 ../../../src/compat/compat.h ../../../src/compat/base64.h \
 ../../../src/compat/inet_aton.h ../../../src/compat/snprintf.h \
 ../../../src/compat/explicit_bzero.h ../../../src/compat/strlcpy.h \
 ../../../src/mod/modvals.h ../../../src/tandem.h \
 .././blowfish.mod/blowfish.h .././blowfish.mod/bf_tab.h
