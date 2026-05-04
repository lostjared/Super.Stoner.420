CC=cc
CFLAGS=  -I./include -I./includes `sdl2-config --cflags` -I. -DHAS_MIXER
LDFLAGS=`sdl2-config --libs` -lSDL2_mixer -lSDL2_ttf -lGLESv2 -lm
SRC_FILES := $(wildcard *.c) src/glad.c
OBJ_FILES := $(SRC_FILES:.c=.o)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<
  
all: $(OBJ_FILES) 
	$(CC) $(OBJ_FILES)  -o SuperStoner420/SuperStoner  $(LDFLAGS)

clean:
	rm -f SuperStoner420/SuperStoner $(OBJ_FILES)
