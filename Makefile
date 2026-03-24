# ─────────────────────────────────────────────────────────────────────────────
# Makefile  –  SimWheel Receiver (Linux port)
#
# Dependencies:
#   nlohmann-json3-dev   (header-only JSON library)
#
#   Ubuntu/Debian:   sudo apt install nlohmann-json3-dev
#   Fedora/RHEL:     sudo dnf install json-devel
#   Arch:            sudo pacman -S nlohmann-json
#
# The uinput kernel module must be loaded:
#   sudo modprobe uinput
#
# Build:
#   make
#
# Run (requires access to /dev/uinput):
#   sudo ./simwheel_receiver
#
#   To avoid running as root every time, add your user to the 'input' group:
#     sudo usermod -aG input $USER
#   Then log out and back in, and run without sudo.
# ─────────────────────────────────────────────────────────────────────────────

CXX      := g++
# -Iinclude  lets you bundle nlohmann/json under include/nlohmann/json.hpp
# (see 'make get-json' below). System install also works without -Iinclude.
CXXFLAGS := -std=c++17 -Wall -Wextra -O2 -Iinclude
TARGET   := simwheel_receiver

SRCS := main.cpp        \
        core.cpp        \
        Setting.cpp     \
        utils.cpp       \
        uinput_functions.cpp \
        wifi.cpp

OBJS := $(SRCS:.cpp=.o)

# ─── Default target ───────────────────────────────────────────────────────────
.PHONY: all
all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^
	@echo ""
	@echo "Build successful: ./$(TARGET)"
	@echo "Run with:         sudo ./$(TARGET)"

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

# ─── Convenience targets ──────────────────────────────────────────────────────
.PHONY: clean
clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: install
install: $(TARGET)
	install -m 755 $(TARGET) /usr/local/bin/$(TARGET)
	@echo "Installed to /usr/local/bin/$(TARGET)"

.PHONY: uninstall
uninstall:
	rm -f /usr/local/bin/$(TARGET)

# Load the uinput kernel module (requires sudo)
.PHONY: load-uinput
load-uinput:
	sudo modprobe uinput
	@echo "uinput module loaded."

# Add current user to the 'input' group so /dev/uinput is accessible without sudo
.PHONY: setup-permissions
setup-permissions:
	sudo usermod -aG input $$USER
	@echo "Added $$USER to the 'input' group."
	@echo "Log out and back in for the change to take effect."

# Download nlohmann/json single-header (MIT licensed) into include/
# Run this once if you don't have the system package installed.
.PHONY: get-json
get-json:
	mkdir -p include/nlohmann
	curl -fsSL https://raw.githubusercontent.com/nlohmann/json/develop/single_include/nlohmann/json.hpp \
	     -o include/nlohmann/json.hpp
	@echo "nlohmann/json downloaded to include/nlohmann/json.hpp"

.PHONY: help
help:
	@echo "Targets:"
	@echo "  all                - Build the receiver (default)"
	@echo "  clean              - Remove build artifacts"
	@echo "  install            - Install to /usr/local/bin"
	@echo "  uninstall          - Remove from /usr/local/bin"
	@echo "  load-uinput        - Load the uinput kernel module (sudo)"
	@echo "  setup-permissions  - Add current user to 'input' group (sudo)"
	@echo "  get-json           - Download nlohmann/json header (needs curl)"
	@echo "  help               - Show this help"
