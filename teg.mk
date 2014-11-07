AR=ar
ARFLAGS=-rsc

TEG_OBJECTS:=obj/teg/config.o obj/teg/io.o obj/teg/miscutil.o obj/teg/video.o obj/teg/xgl.o obj/teg/main.o

lib/libteg.a: $(TEG_OBJECTS)
	@echo Archiving "$@"...
	@$(AR) $(ARFLAGS) "$@" $^
	@if test -f obj/teg/config.debug.o; then $(AR) $(ARFLAGS) $(patsubst %.a,%.debug.a,$@) $(patsubst %.o,%.debug.o,$^); fi
