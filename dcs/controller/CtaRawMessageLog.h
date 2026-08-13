#ifndef CTA_RAW_MESSAGE_LOG_H_
#define CTA_RAW_MESSAGE_LOG_H_

#include <string>

void logCtaRawMessage(
    const std::string& direction,
    const std::string& message,
    const unsigned char *payload,
    unsigned int payloadLength);

#endif
