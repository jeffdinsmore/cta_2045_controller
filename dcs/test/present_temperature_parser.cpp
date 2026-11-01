// Standalone parser regression; no serial hardware required.
#include "controller/UCMImpl.h"
#include "cta2045/processmessage/ProcessMessageUCM.h"
#include <easylogging++.h>
#include <cassert>
#include <vector>
INITIALIZE_EASYLOGGINGPP
struct Receiver : UCMImpl {
    int parsed = 0, statuses = 0, code = -1;
    bool malformed = false;
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
}
