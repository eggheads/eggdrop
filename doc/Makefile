# Makefile for doc/

DOCS = BOTNET BUG-REPORT CONTENTS KNOWN-PROBLEMS \
	MODULES Ode_To_Beldin UPDATES.pre1.3 UPDATES1.3 \
	compiling.FAQ eggdrop.doc patch.howto \
	tcl-commands.doc tricks

MAN1 = man1/eggdrop.1

all: $(DOCS) $(MAN1)

clean:
	@rm -f *~

install:
	@if test ! -d $(DEST)/doc; then \
	echo "Creating a doc subdirectory."; \
		$(INSTALL) -d $(DEST)/doc; \
	fi
	@if test ! -d $(DEST)/doc/man1; then \
		$(INSTALL) -d $(DEST)/doc/man1; \
	fi
	@echo "Copying docs."
	@for i in $(DOCS); do \
		$(INSTALL_DATA) $$i $(DEST)/doc/; \
	done
	@for i in $(MAN1); do \
		$(INSTALL_DATA) $$i $(DEST)/doc/man1/; \
	done

#safety hash
