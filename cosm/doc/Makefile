include ../Makefile.cosm

PAGES = cosm.html \
  os_file.html os_io.html os_math.html os_mem.html os_net.html os_task.html \
  bignum.html buffer.html config.html email.html transform.html \
  http.html language.html log.html security.html time.html

manpages: $(PAGES) html2man.pl
	@mkdir -p man3
	@cd man3; \
	for i in $(PAGES); do \
		echo Building manpages from $$i:; \
		../html2man.pl ../$$i; \
	done

install:
	@mkdir -p $(MAN_DIR)/man3
	@cd man3
	@cp *.3 $(MAN_DIR)/man3

clean:
	@echo "Cleaning..."
	@$(RM) -r man3
