# We only allow compilation on linux!
ifneq ($(shell uname), Linux)
$(error OS must be Linux!)
endif

# Configure output folders for linux, don't work on other systems.
export TMP_DIR		= $(shell pwd)/tmp
export OUT_DIR		= $(shell pwd)/include
export SCRIPT_DIR	= $(shell pwd)/scripts
export MKDIR		= mkdir -p
export RMDIR		= rm -r -f

SAMPLES = $(shell pwd)/samples

# Rules.
.PHONY: all
all: nice

.PHONY: nice
nice: $(TMP_DIR) $(SCRIPT_DIR) $(SAMPLES)

.PHONY: clean
clean:
	$(RMDIR) $(TMP_DIR)

# Create the tmp folder.
$(TMP_DIR):
	$(MKDIR) $@ 

# Create the sm tool.
.PHONY: $(SCRIPT_DIR)
$(SCRIPT_DIR):  $(TMP_DIR)/sm
$(TMP_DIR)/sm: $(SCRIPT_DIR)/nice.template  $(SCRIPT_DIR)/sm.cpp 
	$(CXX) -std=c++2a -o $(TMP_DIR)/sm $(SCRIPT_DIR)/sm.cpp 

.PHONY: $(SAMPLES)
$(SAMPLES):
	$(MAKE) -C $@ $(MAKECMDGOALS)