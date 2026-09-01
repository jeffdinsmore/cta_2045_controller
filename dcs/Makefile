BUILD_DIR := build/debug
PROGRAM := $(BUILD_DIR)/cta2045_controller
COMMODITY_DIAGNOSTIC := $(BUILD_DIR)/cta2045_commodity_read

.PHONY: all controller diagnostic commodity-read schedule run test clean

all: controller

controller:
	cmake -S . -B $(BUILD_DIR) -DCONTROLLER=ON -DCMAKE_BUILD_TYPE=Debug
	cmake --build $(BUILD_DIR) --target cta2045_controller

diagnostic:
	cmake -S . -B $(BUILD_DIR) -DDIAGNOSTIC=ON -DCMAKE_BUILD_TYPE=Debug
	cmake --build $(BUILD_DIR) --target cta2045_commodity_read

commodity-read: diagnostic
	$(COMMODITY_DIAGNOSTIC)

schedule:
	bash controller/create_schedule.sh

run: controller
	cd controller && ../$(PROGRAM)

test: controller
	bash controller/create_schedule.sh
	cd controller && ../$(PROGRAM) --schedule

clean:
	cmake -E remove_directory $(BUILD_DIR)
