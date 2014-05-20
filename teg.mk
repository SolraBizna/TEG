AR=ar
ARFLAGS=-rscD

obj/libteg.a: obj/teg/config.o obj/teg/io.o obj/teg/miscutil.o obj/teg/video.o obj/teg/xgl.o obj/teg/main.o
	@echo Archiving "$@"...
	@$(AR) $(ARFLAGS) "$@" $(filter %.o,$^)
	@test -f obj/teg/config.debug.o && $(AR) $(ARFLAGS) $(patsubst %.a,%.debug.a,"$@") $(patsubst %.o,%.debug.o,$^)
