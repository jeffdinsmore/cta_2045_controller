// Standalone parser regression; no serial hardware required.
#include "controller/UCMImpl.h"
#include "cta2045/processmessage/ProcessMessageUCM.h"
#include <easylogging++.h>
#include <cassert>
#include <vector>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include "cta2045/device/message/Intermediate.h"
#include <cstring>
INITIALIZE_EASYLOGGINGPP
struct Receiver : UCMImpl {
    int parsed = 0, statuses = 0, code = -1;
    bool malformed = false;
    std::vector<unsigned char> setpointBytes;
    void processRawSetpointsReply(const unsigned char* payload, unsigned short length) override {
        setpointBytes.assign(payload, payload + length);
        UCMImpl::processRawSetpointsReply(payload, length);
    }
    void processGetPresentTemperatureResponse(cea2045::cea2045GetPresentTemperatureResponse* m) override {
        ++parsed; assert(m->getPresentTemperature() == 120);
    }
    void processPresentTemperatureStatus(int c, unsigned short, bool bad) override {
        ++statuses; code = c; malformed = bad;
    }
};
struct Link : cea2045::ILinkLayerCommSend {
    int acks = 0;
    int sendRequest(const unsigned char*, int) override { return 0; }
    int sendResponse(const unsigned char*, int) override { return 0; }
    int sendLinkLayerAck() override { return ++acks; }
    int sendLinkLayerNak(cea2045::LinkLayerNakCode) override { return 0; }
};
int main() {
    cea2045::cea2045GetPresentTemperatureResponse temperature = {};
    const int rawValues[] = {12500, -500, -32768, 32767, 0};
    for (int raw : rawValues) {
        temperature.setPresentTemperature(static_cast<unsigned short>(raw));
        assert(temperature.getPresentTemperature() == raw);
        assert(temperature.getPresentTemperatureDegrees() == raw / 100.0);
    }
    Receiver receiver; Link link;
    cea2045::ProcessMessageUCM parser(&receiver);
    // Actual short buffers include header, payload and checksum only.
    auto run = [&](unsigned short length, unsigned char code, unsigned char units) {
        std::vector<unsigned char> data(length + 6, 0);
        data[4] = 3; data[5] = 0x84;
        if (length >= 3) data[6] = code;
        if (length >= 6) data[9] = units;
        if (length >= 8) data[11] = 120;
        auto* header = reinterpret_cast<cea2045::cea2045MessageHeader*>(data.data());
        header->length = htobe16(length);
        parser.processIntermediateMessage(&link, header);
    };
    run(8, 0, 0); run(8, 0, 1);
    assert(receiver.parsed == 2);
    run(3, 1, 0);
    assert(receiver.code == 1 && !receiver.malformed);
    run(7, 0, 0); assert(receiver.malformed);
    run(9, 0, 0); assert(receiver.malformed);
    run(8, 0, 2); assert(receiver.malformed);
    run(2, 0, 0); assert(receiver.code == -1 && receiver.malformed);
    assert(receiver.parsed == 2 && receiver.statuses == 5 && link.acks == 7);
    setenv("CTA_RAW_MESSAGE_LOG_PATH", "setpoint_raw_test.csv", 1);
    { std::ofstream fresh("setpoint_raw_test.csv", std::ios::trunc); }
    const std::vector<std::vector<unsigned char>> payloads = {
        {3, 0x83, 0, 0, 3, 0, 0, 0x8F}, // 143, F, single setpoint
        {3, 0x83, 6},                     // short error
        {3, 0x83, 0, 0, 3}               // unrecognized length
    };
    for (const auto& payload : payloads) {
        std::vector<unsigned char> frame(payload.size() + 6, 0);
        for (size_t i = 0; i < payload.size(); ++i) frame[i + 4] = payload[i];
        auto* header = reinterpret_cast<cea2045::cea2045MessageHeader*>(frame.data());
        header->length = htobe16(payload.size());
        parser.processIntermediateMessage(&link, header);
        assert(receiver.setpointBytes == payload);
    }
    std::ifstream raw("setpoint_raw_test.csv");
    std::ostringstream saved; saved << raw.rdbuf();
    assert(saved.str().find("get_set_setpoints_reply,8,03 83 00 00 03 00 00 8F") != std::string::npos);
    assert(saved.str().find("get_set_setpoints_reply,3,03 83 06") != std::string::npos);
    assert(saved.str().find("get_set_setpoints_reply,5,03 83 00 00 03") != std::string::npos);

    cea2045::Intermediate currentRequest(cea2045::MessageCode::GET_TEMPERATURE_OFFSET, 3, 2);
    assert(currentRequest.getNumBytes() == 8 && currentRequest.getBuffer()[3] == 2);
    cea2045::Intermediate drRequest(cea2045::MessageCode::GET_TEMPERATURE_OFFSET, 3, 2, 10);
    assert(drRequest.getNumBytes() == 9);
    const unsigned char expected[] = {8, 2, 0, 3, 3, 2, 10};
    assert(std::memcmp(drRequest.getBuffer(), expected, 7) == 0);
    const unsigned short checksum = cea2045::Checksum::calculate(drRequest.getBuffer(), 7);
    assert(std::memcmp(drRequest.getBuffer() + 7, &checksum, 2) == 0);
    setenv("CTA_EVENT_LOG_PATH", "offset_events.csv", 1);
    { std::ofstream fresh("offset_events.csv", std::ios::trunc); }
    const std::vector<std::vector<unsigned char>> offsets = {
        {3, 0x82, 0, 4, 0}, {3, 0x82, 0, 6, 1, 10},
        {3, 0x82, 1}, {3, 0x82, 6}, {3, 0x82},
        {3, 0x82, 0, 4}, {3, 0x82, 0, 4, 2},
        {3, 0x82, 0, 0, 0}, {3, 0x82, 0, 255, 1},
        {3, 0x82, 0}, {3, 0x82, 0, 1, 0, 1, 2}
    };
    for (const auto& payload : offsets) {
        std::vector<unsigned char> frame(payload.size() + 6, 0);
        for (size_t i = 0; i < payload.size(); ++i) frame[i + 4] = payload[i];
        auto* header = reinterpret_cast<cea2045::cea2045MessageHeader*>(frame.data());
        header->length = htobe16(payload.size());
        parser.processIntermediateMessage(&link, header);
    }
    std::ifstream offsetFile("offset_events.csv");
    std::ostringstream events; events << offsetFile.rdbuf();
    assert(events.str().find("temperature_offset,temperature_offset_unit,temperature_offset_basic_dr_code") != std::string::npos);
    assert(events.str().find(",4,F,\n") != std::string::npos);
    assert(events.str().find(",6,C,10\n") != std::string::npos);
    assert(events.str().find(",1,command_not_implemented,payload_length=3,,,,,\n") != std::string::npos);
    assert(events.str().find("malformed_or_unrecognized") != std::string::npos);
    assert(events.str().find(",0,F,\n") != std::string::npos);
    assert(events.str().find(",255,C,\n") != std::string::npos);

}
