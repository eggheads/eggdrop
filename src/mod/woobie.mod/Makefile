# Makefile for src/mod/woobie.mod/

doofus:
	@echo ""
	@echo "Let's try this from the right directory..."
	@echo ""
	@cd ../../../; make

clean:
	@rm -f *.o *.so *~

static: ../woobie.o

modules: ../../../woobie.so

../woobie.o: woobie.c ../module.h ../modvals.h ../../eggdrop.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -DMAKING_MODS -c woobie.c
	rm -f ../woobie.o
	mv woobie.o ../

../../../woobie.so: ../woobie.o
	$(LD) -o ../../../woobie.so ../woobie.o
	$(STRIP) ../../../woobie.so

#safety hash
