# Makefile for src/mod/blowfish.mod/

doofus:
	@echo ""
	@echo "Let's try this from the right directory..."
	@echo ""
	@cd ../../..; make

clean:
	@rm -f *.o *.so *~

modules: ../../../blowfish.so

static: ../blowfish.o

blowfish.o: ../module.h ../modvals.h ../../eggdrop.h bf_tab.h blowfish.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -DMAKING_MODS -c blowfish.c

../../../blowfish.so: blowfish.o
	$(LD) -o ../../../blowfish.so blowfish.o
	$(STRIP) ../../../blowfish.so

../blowfish.o: blowfish.o
	@rm -f ../blowfish.o
	@ln -s blowfish.mod/blowfish.o ..

#safety hash
