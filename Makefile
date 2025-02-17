# Makefile

PROJECT ?= 2

.PHONY: all build_project clean

all: build_project
	@echo "All targets for p$(PROJECT) built."

build_project:
	$(MAKE) -C p$(PROJECT)

clean:
	$(MAKE) -C p$(PROJECT) clean
