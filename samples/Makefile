# Special tools.
LDFLAGS_X11			= `pkg-config --cflags --libs x11` -D__X11__
LDFLAGS_SDL			= -lSDL2 -D__SDL__

# Rules.
.PHONY: x11
x11: 
	$(CXX) -o $(BUILD_DIR)/minimal 1_minimal.cpp raster.cpp $(CXXFLAGS) $(LDFLAGS_X11)  

.PHONY: sdl
sdl: 
	$(CXX) -o $(BUILD_DIR)/minimal 1_minimal.cpp $(CXXFLAGS) $(LDFLAGS_SDL) 