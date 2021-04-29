# We only allow compilation on linux!
ifneq ($(shell uname), Linux)
$(error OS must be Linux!)
endif


# Configure output folders for linux, don't work on other systems.
ROOT_DIR			= $(shell pwd)
export INC_DIR		= $(ROOT_DIR)/include
export BUILD_DIR	= $(ROOT_DIR)/build
SCRIPT_DIR			= $(ROOT_DIR)/scripts
SAMPLES 			= $(ROOT_DIR)/samples
NICELIB 			= $(INC_DIR)/nice.hpp


# Configure tools, export C++ related
MKDIR				= mkdir -p
RMDIR				= rm -r -f
export CXX			= g++
export CXXFLAGS		= -std=c++2a -I$(INC_DIR) -g


# Rules.
.PHONY: all
all: nice

.PHONY: nice
nice: dirs tools $(NICELIB) $(SAMPLES)

# Create the tmp folder.
.PHONY: dirs
dirs:
	$(MKDIR) $(BUILD_DIR)

# Create the sm tool.
.PHONY: tools
tools: $(BUILD_DIR)/sm
$(BUILD_DIR)/sm: $(SCRIPT_DIR)/sm.cpp 
	$(CXX) $(CXXFLAGS) -o $(BUILD_DIR)/sm $(SCRIPT_DIR)/sm.cpp 

# Create the nice library
$(NICELIB): $(SCRIPT_DIR)/nice.template
	$(BUILD_DIR)/sm -t $(SCRIPT_DIR)/nice.template >$(NICELIB)
	
.PHONY: $(SAMPLES)
$(SAMPLES):
	$(MAKE) -C $@ $(MAKECMDGOALS)
	
.PHONY: clean
clean:
	$(RMDIR) $(BUILD_DIR)
	$(RM) $(NICELIB)
