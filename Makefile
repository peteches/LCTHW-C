
.ONESHELL:
.PHONY: all
all:
	for i in *; do [[ -r $$i/Makefile ]] && $(MAKE) -C $$i; done

clean:
	for i in *; do [[ -r $$i/Makefile ]] && $(MAKE) -C $$i clean; done
