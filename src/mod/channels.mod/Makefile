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

../channels.o: ../module.h ../modvals.h ../../eggdrop.h cmdschan.c tclchan.c \
userchan.c channels.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -DMAKING_MODS -c channels.c
	mv channels.o ../

../../../channels.so: ../channels.o
	$(LD) -o ../../../channels.so ../channels.o
	$(STRIP) ../../../channels.so

#safety hash
