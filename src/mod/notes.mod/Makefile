# Makefile for src/mod/notes.mod

srcdir = .


doofus:
	@echo "" && \
	@echo "Let's try this from the right directory..." && \
	echo "" && \
	cd ../../../ && $(MAKE)

static: ../notes.o

modules: ../../../notes.$(MOD_EXT)

../notes.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -DMAKING_MODS -c $(srcdir)/notes.c && mv -f notes.o ../

../../../notes.$(MOD_EXT): ../notes.o
	$(LD) -o ../../../notes.$(MOD_EXT) ../notes.o $(XLIBS) $(MODULE_XLIBS) && $(STRIP) ../../../notes.$(MOD_EXT)

depend:
	$(CC) $(CFLAGS) -MM $(srcdir)/notes.c -MT ../notes.o > .depend

clean:
	@rm -f .depend *.o *.$(MOD_EXT) *~

distclean: clean

#safety hash
../notes.o: .././notes.mod/notes.c ../../../src/mod/module.h \
 ../../../src/main.h ../../../config.h ../../../eggint.h ../../../lush.h \
 ../../../src/lang.h ../../../src/eggdrop.h ../../../src/compat/in6.h \
 ../../../src/flags.h ../../../src/cmdt.h ../../../src/tclegg.h \
 ../../../src/tclhash.h ../../../src/chan.h ../../../src/users.h \
 ../../../src/compat/compat.h ../../../src/compat/inet_aton.h \
 ../../../src/compat/snprintf.h ../../../src/compat/memset.h \
 ../../../src/compat/memcpy.h ../../../src/compat/strcasecmp.h \
 ../../../src/compat/strftime.h ../../../src/compat/inet_ntop.h \
 ../../../src/compat/inet_pton.h ../../../src/compat/gethostbyname2.h \
 ../../../src/mod/modvals.h ../../../src/tandem.h .././notes.mod/notes.h \
 .././notes.mod/cmdsnote.c
