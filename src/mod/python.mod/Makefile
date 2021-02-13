# Makefile for src/mod/python.mod/

srcdir = .


doofus:
	@echo "" && \
	echo "Let's try this from the right directory..." && \
	echo "" && \
	cd ../../../ && $(MAKE)

static: ../python.o

modules: ../../../python.$(MOD_EXT)

../python.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) `python3-config --cflags --embed`  `python3-config --embed --ldflags` -DMAKING_MODS -c $(srcdir)/python.c && mv -f python.o ../

../../../python.$(MOD_EXT): ../python.o
	$(LD) $(CFLAGS) -o ../../../python.$(MOD_EXT) ../python.o `python3-config --cflags --embed`  `python3-config --embed --ldflags` $(XLIBS) $(MODULE_XLIBS) && $(STRIP) ../../../python.$(MOD_EXT)

depend:
	$(CC) $(CFLAGS) -MM $(srcdir)/python.c -MT ../python.o > .depend

clean:
	@rm -f .depend *.o *.$(MOD_EXT) *~

distclean: clean

#safety hash
../python.o: .././python.mod/python.c ../../../src/mod/module.h \
 ../../../src/main.h ../../../config.h ../../../eggint.h ../../../lush.h \
 ../../../src/lang.h ../../../src/eggdrop.h ../../../src/compat/in6.h \
 ../../../src/flags.h ../../../src/cmdt.h ../../../src/tclegg.h \
 ../../../src/tclhash.h ../../../src/chan.h ../../../src/users.h \
 ../../../src/compat/compat.h ../../../src/compat/base64.h \
 ../../../src/compat/inet_aton.h ../../../src/compat/snprintf.h \
 ../../../src/compat/gethostbyname2.h \
 ../../../src/compat/explicit_bzero.h ../../../src/compat/strlcpy.h \
 ../../../src/mod/modvals.h ../../../src/tandem.h
