SDL_PATH=/opt/local
CFLAGS=  -I./includes `sdl-config --cflags` -I. -DHAS_MIXER
LDFLAGS=`sdl-config --libs` -lSDL_ttf -lSDL_mixer
CPP_FILES := $(wildcard *.c)
OBJ_FILES := $(addprefix ,$(notdir $(CPP_FILES:.c=.o)))

source/%.o: source/%.cpp
	$(CXX) $(CFLAGS) -c -o $@ $<
  
all: $(OBJ_FILES) 
	$(CXX) $(OBJ_FILES)  -o SuperStoner420/SuperStoner  $(LDFLAGS)

clean:
	rm -f SuperStoner420/SuperStoner *.o 
