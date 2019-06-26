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
../ident.o: ident.c
