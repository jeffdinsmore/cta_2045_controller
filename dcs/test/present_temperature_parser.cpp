// Standalone parser regression; no serial hardware required.
#include "controller/UCMImpl.h"
#include "cta2045/processmessage/ProcessMessageUCM.h"
#include <easylogging++.h>
#include <cassert>
#include <vector>
#include <fstream>
#include <sstream>
#include <cstdlib>
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

}
