# We only allow compilation on linux!
ifneq ($(shell uname), Linux)
$(error OS must be Linux!)
endif

# Check if all required tools are on the system.
REQUIRED = sed
K := $(foreach exec,$(REQUIRED),\
    $(if $(shell which $(exec)),,$(error "$(exec) not found. Please install or add to path.")))

# Configure output folders for linux, don't work on other systems.
export TMP_DIR		= $(shell pwd)/tmp
export OUT_DIR		= $(shell pwd)/include
export MKDIR		= mkdir -p
export RMDIR		= rm -r -f

# Build directories (all)
BUILDDIRS = $(TMP_DIR)

# Targets are subdirs.
SUBDIRS = src samples

# Rules.
.PHONY: all clean $(BUILDDIRS) $(SUBDIRS)

all: $(BUILDDIRS) $(SUBDIRS)

clean:
	$(RMDIR) $(TMP_DIR)

$(SUBDIRS):
	$(MAKE) -C $@ $(MAKECMDGOALS)

$(BUILDDIRS):
	$(MKDIR) $@ 