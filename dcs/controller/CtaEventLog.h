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
    const std::string& eventId = "");

#endif
