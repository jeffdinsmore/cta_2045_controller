// Standalone regression test: compile with controller sources (except main.cpp)
// and cta2045 sources. Run from a temporary directory; no hardware is used.
#include "controller/UCMImpl.h"
#include <easylogging++.h>
#include <cassert>
#include <cstring>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <algorithm>
INITIALIZE_EASYLOGGINGPP

std::string readFile(const char* path) {
    std::ifstream input(path);
    std::ostringstream output;
    output << input.rdbuf();
    return output.str();
}
int main() {
    setenv("CTA_DEVICE_INFO_LOG_PATH", "public.csv", 1);
    setenv("CTA_PRIVATE_DEVICE_INFO_LOG_PATH", "private.csv", 1);
    setenv("CTA_EQUIPMENT_ID", "wh_test123", 1);
    cea2045::cea2045DeviceInfoResponse response = {};
    response.length = htobe16(53);
    response.opCode1 = 1; response.opCode2 = 0x81;
    response.version[0] = 0x42;
    response.vendorId = htobe16(0xABCD);
    response.deviceType = htobe16(3);
    response.deviceRevision[0] = 0x77;
    response.capability[3] = 0x40;
    std::memcpy(response.modelNumber, "PRIVATE_MODEL", 13);
    std::memcpy(response.serialNumber, "PRIVATE_SERIAL", 14);
    response.firmwareYear20xx = 26;
    response.firmwareDay = 1;
    UCMImpl controller;
    controller.processDeviceInfoResponse(&response);
    cea2045::cea2045GetSetpointsResponse2 setpoints = {};
    setpoints.units = 0;
    setpoints.setDeviceType(3);
    setpoints.setSetpoint1(12500);
    setpoints.setSetpoint2(13000);
    controller.processGetSetpointsResponse(&setpoints);
    controller.finishStartupSetpoints("success");
    std::string publicData = readFile("public.csv");
    assert(publicData.find("setpoint1_raw,setpoint2_raw") != std::string::npos);
    assert(publicData.find(",success,3,0,12500,13000") != std::string::npos);
    setpoints.setSetpoint1(14000);
    controller.processGetSetpointsResponse(&setpoints);
    controller.finishStartupSetpoints("success");
    assert(readFile("public.csv") == publicData);
    std::string privateData = readFile("private.csv");
    assert(publicData.find("wh_test123") != std::string::npos);
    assert(publicData.find("device_type_name") != std::string::npos);
    assert(publicData.find("00000040") != std::string::npos);
    const char* forbidden[] = {"vendor_id", "ABCD", "model_number", "PRIVATE_MODEL",
        "serial_number", "PRIVATE_SERIAL", "firmware", "device_revision"};
    for (const char* value : forbidden)
        assert(publicData.find(value) == std::string::npos);
    assert(privateData.find("PRIVATE_MODEL") != std::string::npos);
    assert(privateData.find("0xABCD") != std::string::npos);
    assert(privateData.find("device_revision_hex") != std::string::npos);
    std::istringstream rows(publicData);
    std::string header, row;
    std::getline(rows, header); std::getline(rows, row);
    assert(std::count(header.begin(), header.end(), ',') == 17);
    assert(std::count(row.begin(), row.end(), ',') == 17);
    // Unsupported optional fields and no configured equipment identity remain valid.
    unsetenv("CTA_EQUIPMENT_ID");
    unsetenv("CTA_PRIVATE_DEVICE_INFO_LOG_PATH");
    response.length = htobe16(3);
    controller.processDeviceInfoResponse(&response);
    publicData = readFile("public.csv");
    assert(publicData.find("wh_test123") == std::string::npos);
    std::istringstream shortRows(publicData);
    std::getline(shortRows, header); std::getline(shortRows, row);
    assert(std::count(row.begin(), row.end(), ',') == 17);
    UCMImpl unsupported;
    unsupported.processDeviceInfoResponse(&response);
    unsupported.finishStartupSetpoints("command_not_implemented");
    assert(readFile("public.csv").find(",command_not_implemented,,,,") != std::string::npos);
    UCMImpl missing;
    missing.finishStartupSetpoints("timeout");
    publicData = readFile("public.csv");
    assert(publicData.find("no_device_information") != std::string::npos);
    std::istringstream missingRows(publicData);
    std::getline(missingRows, header); std::getline(missingRows, row);
    assert(std::count(row.begin(), row.end(), ',') == 17);
    setenv("CTA_EVENT_LOG_PATH", "temperature_events.csv", 1);
    cea2045::cea2045GetPresentTemperatureResponse temperature = {};
    temperature.units = 1;
    temperature.setDeviceType(3);
    temperature.setPresentTemperature(static_cast<unsigned short>(-500));
    controller.processGetPresentTemperatureResponse(&temperature);
    const std::string events = readFile("temperature_events.csv");
    assert(events.find("present_temperature=-5.00") != std::string::npos);
    assert(events.find("temperature_unit=C") != std::string::npos);

}
