# Makefile for src/mod/blowfish.mod/

doofus:
	@echo ""
	@echo "Let's try this from the right directory..."
	@echo ""
	@cd ../../../; make

clean:
	@rm -f *.o *.so *~

static: ../blowfish.o

modules: ../../../blowfish.so

../blowfish.o: bf_tab.h blowfish.c blowfish.h ../module.h ../modvals.h \
../../eggdrop.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -DMAKING_MODS -c blowfish.c
	rm -f ../blowfish.o
	mv blowfish.o ../

../../../blowfish.so: ../blowfish.o
	$(LD) -o ../../../blowfish.so ../blowfish.o
	$(STRIP) ../../../blowfish.so

#safety hash
