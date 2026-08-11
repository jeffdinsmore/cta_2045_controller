#ifndef CEA2045_CEA2045_MESSAGE_CEA2045MESSAGEGETADVANCEDLOADUPRESPONSE_H_
#define CEA2045_CEA2045_MESSAGE_CEA2045MESSAGEGETADVANCEDLOADUPRESPONSE_H_

#include "CEA2045Message.h"
#include <endian.h>

namespace cea2045 {

struct cea2045GetAdvancedLoadUpResponse
{
	cea2045MessageHeader header;
	unsigned char opCode1;
	unsigned char opCode2;
	unsigned char responseCode;
	unsigned short eventDuration;
	unsigned short value;
	unsigned char units;

	unsigned short getEventDuration() const
	{
		return be16toh(eventDuration);
	}

	unsigned short getValue() const
	{
		return be16toh(value);
	}
} __attribute__((packed));

}
#endif /* CEA2045_CEA2045_MESSAGE_CEA2045MESSAGEGETADVANCEDLOADUPRESPONSE_H_ */
