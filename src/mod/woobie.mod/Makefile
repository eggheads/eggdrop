# Makefile for src/mod/woobie.mod/

srcdir = .


doofus:
	@echo "" && \
	echo "Let's try this from the right directory..." && \
	echo "" && \
	cd ../../../ && $(MAKE)

static: ../woobie.o

modules: ../../../woobie.$(MOD_EXT)

../woobie.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -DMAKING_MODS -c $(srcdir)/woobie.c && mv -f woobie.o ../

../../../woobie.$(MOD_EXT): ../woobie.o
	$(LD) $(CFLAGS) -o ../../../woobie.$(MOD_EXT) ../woobie.o $(XLIBS) $(MODULE_XLIBS) && $(STRIP) ../../../woobie.$(MOD_EXT)

depend:
	$(CC) $(CFLAGS) -MM $(srcdir)/woobie.c -MT ../woobie.o > .depend

clean:
	@rm -f .depend *.o *.$(MOD_EXT) *~

distclean: clean

#safety hash
../woobie.o: woobie.c ../../../src/mod/module.h ../../../src/main.h \
 ../../../config.h ../../../eggint.h ../../../lush.h ../../../src/lang.h \
 ../../../src/eggdrop.h ../../../src/compat/in6.h ../../../src/flags.h \
 ../../../src/cmdt.h ../../../src/tclegg.h ../../../src/tclhash.h \
 ../../../src/chan.h ../../../src/users.h ../../../src/compat/compat.h \
 ../../../src/compat/base64.h ../../../src/compat/inet_aton.h \
 ../../../src/compat/snprintf.h ../../../src/compat/explicit_bzero.h \
 ../../../src/compat/strlcpy.h ../../../src/mod/modvals.h \
 ../../../src/tandem.h
