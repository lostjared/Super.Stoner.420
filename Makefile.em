SDL_PATH=/opt/local
CXX=em++
CC=emcc
CFLAGS=  -I./includes -s USE_SDL=2 -sUSE_SDL_MIXER=2  -I. -DHAS_MIXER
LDFLAGS= -s USE_SDL=2 -s USE_SDL_MIXER=2 -s  --preload-file assets
CPP_FILES := $(wildcard *.c)
OBJ_FILES := $(addprefix ,$(notdir $(CPP_FILES:.c=.o)))

source/%.o: source/%.cpp
	$(CXX) $(CFLAGS) -c -o $@ $<
  
all: $(OBJ_FILES) 
	$(CXX) $(OBJ_FILES)  -o SuperStoner.html  $(LDFLAGS)

clean:
	rm -f SuperStoner420/SuperStoner *.o 
