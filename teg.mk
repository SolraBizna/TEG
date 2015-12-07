ifndef TEG_OBJECTS
TEG_OBJECTS:=obj/teg/config.o obj/teg/io.o obj/teg/miscutil.o obj/teg/video.o obj/teg/xgl.o obj/teg/netsock.o obj/teg/postinit.o obj/teg/main.o
endif

lib/libteg.a: $(TEG_OBJECTS)
	@echo Archiving "$@"...
	@$(AR) $(ARFLAGS) "$@" $^

lib/libteg.debug.a: $(patsubst %.o,%.debug.o,$(TEG_OBJECTS))
	@echo Archiving "$@"...
	@$(AR) $(ARFLAGS) "$@" $^
