BUILD_DIR := build/debug
PROGRAM := $(BUILD_DIR)/cta2045_controller

.PHONY: all controller schedule run test clean

all: controller

controller:
	cmake -S . -B $(BUILD_DIR) -DCONTROLLER=ON -DCMAKE_BUILD_TYPE=Debug
	cmake --build $(BUILD_DIR) --target cta2045_controller

schedule:
	bash controller/create_schedule.sh

run: controller
	cd controller && ../$(PROGRAM)

test: controller
	bash controller/create_schedule.sh
	cd controller && ../$(PROGRAM)

clean:
	cmake -E remove_directory $(BUILD_DIR)
