	
    #@defines
	PREFIX ?= /usr
	BIN_DIR ?= $(PREFIX)/bin
	SHARE_DIR ?= $(PREFIX)/share
	DESKAPPS_DIR ?= $(SHARE_DIR)/applications
	PIXMAPS_DIR ?= $(SHARE_DIR)/pixmaps
	MAN_DIR ?= $(SHARE_DIR)/man/man1

	# set name
	NAME = jalv.select
	VER = 0.9
	# create debian package
	DEBNAME = jalvselect_$(VER)
	CREATEDEB = dh_make -y -s -n -e $(USER)@org -p $(DEBNAME) -c gpl >/dev/null
	DIRS = $(BIN_DIR)  $(DESKAPPS_DIR)  $(PIXMAPS_DIR)  $(MAN_DIR)
	BUILDDEB = dpkg-buildpackage -rfakeroot -b 2>/dev/null | grep dpkg-deb 
	# set compile flags
	CXXFLAGS ?= -std=c++11 `pkg-config gtkmm-2.4 lilv-0 --cflags` 
	LDFLAGS ?=  -lX11 `pkg-config gtkmm-2.4 lilv-0 --libs` 
	# invoke build files
	OBJECTS = $(NAME).cpp 
	## output style (bash colours)
	BLUE = "\033[1;34m"
	RED =  "\033[1;31m"
	NONE = "\033[0m"
	## check if config.h is valid
	CONFIG_H := $(shell cat config.h 2>/dev/null | grep PIXMAPS_DIR | grep -oP '[^"]*"\K[^"]*')

.PHONY : all clean dist-clean install tar deb uninstall 

all : check

config.h : 
ifneq ($(CONFIG_H), $(PIXMAPS_DIR))
	@echo '#define VERSION  "$(VER)"' > config.h 
	@echo '#define PIXMAPS_DIR  "$(PIXMAPS_DIR)"' >> config.h 
endif

check : $(NAME)
	@if [ -f $(NAME) ]; then echo $(BLUE)"build finish, now run make install"; \
	else echo -e $(RED)"sorry, build failed" $(NONE); fi
	@echo $(NONE)

clean :
	@rm -f $(NAME)
	@rm -f config.h
	@echo ". ." $(BLUE)", clean"$(NONE)

dist-clean :
	@rm -rf ./debian
	@rm -f $(NAME)
	@rm -f config.h
	@echo ". ." $(BLUE)", clean"$(NONE)

install : all
	@mkdir -p $(DESTDIR)$(BIN_DIR)
	@mkdir -p $(DESTDIR)$(DESKAPPS_DIR)
	@mkdir -p $(DESTDIR)$(PIXMAPS_DIR)
	@mkdir -p $(DESTDIR)$(MAN_DIR)
	install $(NAME) $(DESTDIR)$(BIN_DIR)
	cp $(NAME).desktop $(DESTDIR)$(DESKAPPS_DIR)
	cp lv2.png $(DESTDIR)$(PIXMAPS_DIR)
	cp lv2_16.png $(DESTDIR)$(PIXMAPS_DIR)
	cp jalv.select.1 $(DESTDIR)$(MAN_DIR)
	cp jalv.select.fr.1 $(DESTDIR)$(MAN_DIR)
	gzip $(DESTDIR)$(MAN_DIR)/jalv.select.1
	gzip $(DESTDIR)$(MAN_DIR)/jalv.select.fr.1
	@echo ". ." $(BLUE)", done"$(NONE)

    #@create tar ball
tar : clean
	@cd ../ && \
	tar -cf $(NAME)-$(VER).lv2.tar.bz2 --bzip2 $(NAME)
	@echo $(BLUE)"build "$(NAME)"-"$(VER)".lv2.tar.bz2"$(NONE)

    #@build a debian packet for all arch
deb :
	@rm -rf ./debian
	@echo $(BLUE)"create ./debian"$(NONE)
	-@ $(CREATEDEB)
	@ #@echo $(BLUE)"touch ./debian/dirs"$(NONE)
	@ #-@echo $(DIRS) > ./debian/dirs
	@echo $(BLUE)"try to build debian package, that may take some time"$(NONE)
	-@ if $(BUILDDEB); then echo ". ." $(BLUE)", done"$(NONE); else \
     echo -e $(RED)"sorry, build fail"$(NONE); fi

uninstall :
	rm -rf $(BIN_DIR)/$(NAME) $(DESKAPPS_DIR)/$(NAME).desktop $(PIXMAPS_DIR)/lv2.png $(PIXMAPS_DIR)/lv2_16.png $(MAN_DIR)/jalv.select.1.gz $(MAN_DIR)/jalv.select.fr.1.gz
	@echo ". ." $(BLUE)", done"$(NONE)

$(NAME) : config.h $(OBJECTS) $(NAME).h
	$(CXX) $(CXXFLAGS) $(OBJECTS) $(LDFLAGS) -o $(NAME)
