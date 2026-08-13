#include "CtaRawMessageLog.h"

#include <chrono>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <mutex>
#include <sstream>
#include <sys/stat.h>

namespace
{
const char* LOG_DIRECTORY = "logs";
std::mutex rawMessageLogMutex;

const char* rawMessageLogPath()
{
    const char* configured = std::getenv("CTA_RAW_MESSAGE_LOG_PATH");
    return configured != NULL && configured[0] != '\0'
        ? configured
        : "logs/cta_raw_messages.csv";
}

std::string currentPacificTimestamp()
{
    const std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
    const std::chrono::milliseconds sinceEpoch =
        std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch());
    const std::time_t seconds = std::chrono::system_clock::to_time_t(now);
    std::tm local;
#ifdef _WIN32
    localtime_s(&local, &seconds);
#else
    localtime_r(&seconds, &local);
#endif

    char utcOffset[8] = "";
    std::strftime(utcOffset, sizeof(utcOffset), "%z", &local);
    std::string formattedOffset(utcOffset);
    if (formattedOffset.size() == 5)
        formattedOffset.insert(3, ":");

    std::ostringstream timestamp;
    timestamp << std::put_time(&local, "%Y-%m-%dT%H:%M:%S")
              << '.' << std::setfill('0') << std::setw(3)
              << (sinceEpoch.count() % 1000) << formattedOffset;
    return timestamp.str();
}

std::string bytesToHex(const unsigned char *bytes, unsigned int count)
{
    std::ostringstream value;
    value << std::uppercase << std::hex << std::setfill('0');
    for (unsigned int index = 0; index < count; index++)
    {
        if (index != 0)
            value << ' ';
        value << std::setw(2) << static_cast<unsigned int>(bytes[index]);
    }
    return value.str();
}
}

void logCtaRawMessage(
    const std::string& direction,
    const std::string& message,
    const unsigned char *payload,
    unsigned int payloadLength)
{
    std::lock_guard<std::mutex> lock(rawMessageLogMutex);
    mkdir(LOG_DIRECTORY, 0755);

    const char* path = rawMessageLogPath();
    std::ifstream existing(path, std::ios::binary | std::ios::ate);
    const bool needsHeader = !existing.is_open() || existing.tellg() == 0;
    existing.close();

    std::ofstream output(path, std::ios_base::out | std::ios_base::app);
    if (!output.is_open())
        return;
    if (needsHeader)
        output << "timestamp_pacific,direction,message,payload_length,payload_hex\n";

    output << currentPacificTimestamp() << ','
           << direction << ','
           << message << ','
           << payloadLength << ','
           << bytesToHex(payload, payloadLength) << '\n';
    output.flush();
}
