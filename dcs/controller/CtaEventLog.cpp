#include "CtaEventLog.h"

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
std::mutex eventLogMutex;

const char* eventLogPath()
{
    const char* configured = std::getenv("CTA_EVENT_LOG_PATH");
    return configured != NULL && configured[0] != '\0'
        ? configured
        : "logs/cta_events.csv";
}

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
}

void logCtaEvent(
    const std::string& event,
    const std::string& direction,
    const std::string& command,
    const std::string& result,
    const std::string& argument,
    const std::string& details,
    const std::string& eventId,
    const std::string& source,
    const std::string& opcode1,
    const std::string& opcode2,
    const std::string& nakCode,
    const std::string& nakReason,
    const std::string& operationalState,
    const std::string& operationalStateName,
    const std::string& intermediateResponseCode,
    const std::string& intermediateResponseName)
{
    std::lock_guard<std::mutex> lock(eventLogMutex);
    mkdir(LOG_DIRECTORY, 0755);

    const char* path = eventLogPath();
    std::ifstream existing(path, std::ios::binary | std::ios::ate);
    const bool needsHeader = !existing.is_open() || existing.tellg() == 0;
    existing.close();

    std::ofstream output(path, std::ios_base::out | std::ios_base::app);
    if (!output.is_open())
        return;
    if (needsHeader)
        output << "timestamp_pacific,event_id,event,direction,command,result,"
                  "argument,source,opcode1,opcode2,nak_code,nak_reason,"
                  "operational_state,operational_state_name,"
                  "intermediate_response_code,intermediate_response_name,details\n";

    output << csvEscape(currentPacificTimestamp()) << ','
           << csvEscape(eventId) << ','
           << csvEscape(event) << ','
           << csvEscape(direction) << ','
           << csvEscape(command) << ','
           << csvEscape(result) << ','
           << csvEscape(argument) << ','
           << csvEscape(source) << ','
           << csvEscape(opcode1) << ','
           << csvEscape(opcode2) << ','
           << csvEscape(nakCode) << ','
           << csvEscape(nakReason) << ','
           << csvEscape(operationalState) << ','
           << csvEscape(operationalStateName) << ','
           << csvEscape(intermediateResponseCode) << ','
           << csvEscape(intermediateResponseName) << ','
           << csvEscape(details) << '\n';
    output.flush();
}
