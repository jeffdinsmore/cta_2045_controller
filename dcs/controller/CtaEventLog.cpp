#include "CtaEventLog.h"

#include <chrono>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <mutex>
#include <sstream>
#include <sys/stat.h>

namespace
{
const char* LOG_DIRECTORY = "logs";
const char* EVENT_LOG_PATH = "logs/cta_events.csv";
std::mutex eventLogMutex;

std::string csvEscape(const std::string& value)
{
    if (value.find_first_of(",\"\r\n") == std::string::npos)
        return value;

    std::string escaped = "\"";
    for (std::string::const_iterator character = value.begin(); character != value.end(); ++character)
    {
        if (*character == '"')
            escaped += "\"\"";
        else
            escaped += *character;
    }
    escaped += '"';
    return escaped;
}

std::string currentUtcTimestamp()
{
    const std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
    const std::chrono::milliseconds sinceEpoch =
        std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch());
    const std::time_t seconds = std::chrono::system_clock::to_time_t(now);
    std::tm utc;
#ifdef _WIN32
    gmtime_s(&utc, &seconds);
#else
    gmtime_r(&seconds, &utc);
#endif

    std::ostringstream timestamp;
    timestamp << std::put_time(&utc, "%Y-%m-%dT%H:%M:%S")
              << '.' << std::setfill('0') << std::setw(3)
              << (sinceEpoch.count() % 1000) << 'Z';
    return timestamp.str();
}
}

void logCtaEvent(
    const std::string& event,
    const std::string& direction,
    const std::string& command,
    const std::string& result,
    const std::string& argument,
    const std::string& details,
    const std::string& eventId)
{
    std::lock_guard<std::mutex> lock(eventLogMutex);
    mkdir(LOG_DIRECTORY, 0755);

    std::ifstream existing(EVENT_LOG_PATH, std::ios::binary | std::ios::ate);
    const bool needsHeader = !existing.is_open() || existing.tellg() == 0;
    existing.close();

    std::ofstream output(EVENT_LOG_PATH, std::ios_base::out | std::ios_base::app);
    if (!output.is_open())
        return;
    if (needsHeader)
        output << "timestamp_utc,event_id,event,direction,command,result,argument,details\n";

    output << csvEscape(currentUtcTimestamp()) << ','
           << csvEscape(eventId) << ','
           << csvEscape(event) << ','
           << csvEscape(direction) << ','
           << csvEscape(command) << ','
           << csvEscape(result) << ','
           << csvEscape(argument) << ','
           << csvEscape(details) << '\n';
    output.flush();
}
