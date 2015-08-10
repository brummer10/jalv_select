	
    #@defines
	PREFIX = /usr
	BIN_DIR = $(PREFIX)/bin
	SHARE_DIR = $(PREFIX)/share
	DESKAPPS_DIR = $(SHARE_DIR)/applications
	PIXMAPS_DIR = $(SHARE_DIR)/pixmaps

	# set name
	NAME = jalv.select
	VER = 0.1
	# create debian package
	DEBNAME = jalvselect_$(VER)
	CREATEDEB = yes '' | dh_make -s -n -e $(USER)@org -p $(DEBNAME) -c gpl >/dev/null
	DIRS = $(BIN_DIR)  $(DESKAPPS_DIR)  $(PIXMAPS_DIR) 
	BUILDDEB = dpkg-buildpackage -rfakeroot -b 2>/dev/null | grep dpkg-deb 
	# set compile flags
	CXXFLAGS = `pkg-config gtkmm-2.4 lilv-0 --cflags` 
	LDFLAGS = `pkg-config gtkmm-2.4 lilv-0 --libs`
	# invoke build files
	OBJECTS = $(NAME).cpp 
	## output style (bash colours)
	BLUE = "\033[1;34m"
	RED =  "\033[1;31m"
	NONE = "\033[0m"

.PHONY : all clean install uninstall 

all : $(NAME)
	@if [ -f $(NAME) ]; then echo $(BLUE)"build finish, now run make install"; \
	else echo $(RED)"sorry, build failed"; fi
	@echo $(NONE)

clean :
	@rm -f $(NAME)
	@echo ". ." $(BLUE)", clean"$(NONE)

dist-clean :
	@rm -rf ./debian
	@rm -f $(NAME)
	@echo ". ." $(BLUE)", clean"$(NONE)

install : all
	@mkdir -p $(DESTDIR)$(BIN_DIR)
	@mkdir -p $(DESTDIR)$(DESKAPPS_DIR)
	@mkdir -p $(DESTDIR)$(PIXMAPS_DIR)
	install $(NAME) $(DESTDIR)$(BIN_DIR)
	install $(NAME).desktop $(DESTDIR)$(DESKAPPS_DIR)
	install lv2.png $(DESTDIR)$(PIXMAPS_DIR)
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
     echo $(RED)"sorry, build fail"$(NONE); fi

uninstall :
	rm -rf $(BIN_DIR)/$(NAME) $(DESKAPPS_DIR)/$(NAME).desktop $(PIXMAPS_DIR)/lv2.png
	@echo ". ." $(BLUE)", done"$(NONE)

$(NAME) : 
	$(CXX) $(CXXFLAGS) $(OBJECTS) $(LDFLAGS) -o $(NAME)
