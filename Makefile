DESTDIR = /usr/bin
BINNAME = gbooks-scrapper

build:
	make -C jsmn libjsmn.a
	$(CC) main.c -o gbooks-scrapper -lcurl jsmn/libjsmn.a

.PHONY: install
install: 
	cp gbooks-scrapper $(DESTDIR)/$(BINNAME)

.PHONY: uninstall
uninstall:
	rm -f $(DESTDIR)/$(BINNAME)

.PHONY: clean
clean:
	make -C jsmn clean
	rm -f gbooks-scrapper