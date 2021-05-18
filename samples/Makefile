# Special tools.
LDFLAGS_X11			= `pkg-config --cflags --libs x11` -D__X11__

# Rules.
.PHONY: x11
x11: 
	$(CXX) -o $(BUILD_DIR)/minimal 1_minimal.cpp $(CXXFLAGS) $(LDFLAGS_X11)  
