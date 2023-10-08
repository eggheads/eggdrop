# Makefile.in for src/mod/pbkdf2.mod/

srcdir = .


doofus:
	@echo "" && \
	echo "Let's try this from the right directory..." && \
	echo "" && \
	cd ../../../ && $(MAKE)

static: ../pbkdf2.o

modules: ../../../pbkdf2.$(MOD_EXT)

../pbkdf2.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -DMAKING_MODS -c $(srcdir)/pbkdf2.c && mv -f pbkdf2.o ../

../../../pbkdf2.$(MOD_EXT): ../pbkdf2.o
	$(LD) $(CFLAGS) -o ../../../pbkdf2.$(MOD_EXT) ../pbkdf2.o $(XLIBS) $(MODULE_XLIBS) && $(STRIP) ../../../pbkdf2.$(MOD_EXT)

depend:
	$(CC) $(CFLAGS) -MM $(srcdir)/pbkdf2.c -MT ../pbkdf2.o > .depend

clean:
	@rm -f .depend *.o *.$(MOD_EXT) *~

distclean: clean

#safety hash
../pbkdf2.o: .././pbkdf2.mod/pbkdf2.c ../../../src/mod/module.h \
 ../../../src/main.h ../../../config.h ../../../eggint.h ../../../lush.h \
 ../../../src/lang.h ../../../src/eggdrop.h ../../../src/compat/in6.h \
 ../../../src/flags.h ../../../src/cmdt.h ../../../src/tclegg.h \
 ../../../src/tclhash.h ../../../src/chan.h ../../../src/users.h \
 ../../../src/compat/compat.h ../../../src/compat/base64.h \
 ../../../src/compat/inet_aton.h ../../../src/compat/snprintf.h \
 ../../../src/compat/explicit_bzero.h ../../../src/compat/strlcpy.h \
 ../../../src/mod/modvals.h ../../../src/tandem.h
