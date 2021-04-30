# Special tools.
LDFLAGS_SDL			= -L$(BUILD_DIR) -lSDL2
LDFLAGS_X11			= `pkg-config --cflags --libs x11` -D__X11__

# Rules.
.PHONY: x11
x11: 
	$(CXX) $(CXXFLAGS) $(LDFLAGS_X11) -o $(BUILD_DIR)/minimal 1_minimal.cpp
	# $(CXX) $(CXXFLAGS) -o $(BUILD_DIR)/scrible 2_scrible.cpp
	# $(CXX) $(CXXFLAGS) -o $(BUILD_DIR)/calculator 3_calculator.cpp
