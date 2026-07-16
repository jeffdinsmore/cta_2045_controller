BUILD_DIR := build/debug
PROGRAM := $(BUILD_DIR)/sample2

.PHONY: all sample2 run clean

all: sample2

sample2:
	cmake -S . -B $(BUILD_DIR) -DSAMPLE=ON -DCMAKE_BUILD_TYPE=Debug
	cmake --build $(BUILD_DIR) --target sample2

run: sample2
	cd sample2 && ../$(PROGRAM)

clean:
	@if [ -f "$(BUILD_DIR)/CMakeCache.txt" ]; then \
		cmake --build $(BUILD_DIR) --target clean; \
	else \
		echo "Nothing to clean; $(BUILD_DIR) is not configured."; \
	fi
