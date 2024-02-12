# Makefile for src/mod/zig.mod/

srcdir = .

doofus:
	@echo "" && \
	echo "Let's try this from the right directory..." && \
	echo "" && \
	cd ../../../ && $(MAKE)

static: ../zig.o

modules: ../../../zig.$(MOD_EXT)

../zig.o:
	zig build-lib -fPIC -lc -isystem ../../.. zig.zig
	mv libzig.a.o ../zig.o

../../../zig.$(MOD_EXT): ../zig.o
	$(LD) $(CFLAGS) -o ../../../zig.$(MOD_EXT) ../zig.o $(XLIBS) $(MODULE_XLIBS)
	$(STRIP) ../../../zig.$(MOD_EXT)

clean:
	@rm -f *.o *.$(MOD_EXT)

distclean: clean
