#ifndef CEA2045_SETCAPABILITYBIT_H_
#define CEA2045_SETCAPABILITYBIT_H_

#include "Message.h"
#include "../../message/CEA2045Message.h"
#include "../../util/Checksum.h"

namespace cea2045
{

class SetCapabilityBit : public Message
{
private:
	struct SetCapabilityBitMessage
	{
		cea2045MessageHeader header;
		unsigned char opCode1;
		unsigned char opCode2;
		unsigned char bitNumber;
		unsigned char setOrUnset;
		unsigned short checksum;

		void setLength()
		{
			header.length = htobe16(4);
		}

		void setChecksum()
		{
			checksum = Checksum::calculate(
				(unsigned char *)this,
				sizeof(SetCapabilityBitMessage) - sizeof(unsigned short));
		}
	} __attribute__((packed));

	SetCapabilityBitMessage m_msg;

public:
	SetCapabilityBit(unsigned char bitNumber, bool enabled);
	virtual ~SetCapabilityBit();

	virtual int getNumBytes();
	virtual unsigned char *getBuffer();
};

} /* namespace cea2045 */

#endif /* CEA2045_SETCAPABILITYBIT_H_ */
