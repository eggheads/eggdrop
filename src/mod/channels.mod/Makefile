# Makefile for src/mod/channels.mod/

doofus:
	@echo ""
	@echo "Let's try this from the right directory..."
	@echo ""
	@cd ../../../; make

clean:
	@rm -f *.o *.so *~

static: ../channels.o

modules: ../../../channels.so

../channels.o: channels.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -DMAKING_MODS -c channels.c
	rm -f ../channels.o
	mv channels.o ../

../../../channels.so: ../channels.o
	$(LD) -o ../../../channels.so ../channels.o
	$(STRIP) ../../../channels.so

channels.c: cmdschan.c tclchan.c userchan.c channels.h \
../module.h ../modvals.h ../../eggdrop.h

#safety hash
