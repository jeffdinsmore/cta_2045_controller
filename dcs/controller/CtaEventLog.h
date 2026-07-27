#ifndef CTA_EVENT_LOG_H_
#define CTA_EVENT_LOG_H_

#include <string>

void logCtaEvent(
    const std::string& event,
    const std::string& direction,
    const std::string& command,
    const std::string& result,
    const std::string& argument = "",
    const std::string& details = "",
    const std::string& eventId = "",
    const std::string& source = "",
    const std::string& opcode1 = "",
    const std::string& opcode2 = "",
    const std::string& nakCode = "",
    const std::string& nakReason = "",
    const std::string& operationalState = "",
    const std::string& operationalStateName = "",
    const std::string& intermediateResponseCode = "",
    const std::string& intermediateResponseName = "");

#endif
