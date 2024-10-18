# Makefile for src/mod/webui.mod/

srcdir = .


doofus:
	@echo "" && \
	echo "Let's try this from the right directory..." && \
	echo "" && \
	cd ../../../ && $(MAKE)

static: ../webui.o

modules: ../../../webui.$(MOD_EXT)

../webui.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -DMAKING_MODS -c $(srcdir)/webui.c && mv -f webui.o ../

../../../webui.$(MOD_EXT): ../webui.o
	$(LD) $(CFLAGS) -o ../../../webui.$(MOD_EXT) ../webui.o $(XLIBS) $(MODULE_XLIBS) && $(STRIP) ../../../webui.$(MOD_EXT)

depend:
	$(CC) $(CFLAGS) -MM $(srcdir)/webui.c -MT ../webui.o > .depend

clean:
	@rm -f .depend *.o *.$(MOD_EXT) *~

distclean: clean

install: install-text

install-text:
	if test ! -f $(DEST)/text/webui.html; then \
		$(INSTALL_DATA) $(srcdir)/text/webui.html $(DEST)/text/; \
	fi

#safety hash
../webui.o: .././webui.mod/webui.c ../../../src/mod/module.h \
 ../../../src/main.h ../../../config.h ../../../eggint.h ../../../lush.h \
 ../../../src/lang.h ../../../src/eggdrop.h ../../../src/compat/in6.h \
 ../../../src/flags.h ../../../src/cmdt.h ../../../src/tclegg.h \
 ../../../src/tclhash.h ../../../src/chan.h ../../../src/users.h \
 ../../../src/compat/compat.h ../../../src/compat/base64.h \
 ../../../src/compat/inet_aton.h ../../../src/compat/snprintf.h \
 ../../../src/compat/explicit_bzero.h ../../../src/compat/strlcpy.h \
 ../../../src/mod/modvals.h ../../../src/tandem.h ../../../src/version.h
