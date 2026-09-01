#include "SetCapabilityBit.h"

namespace cea2045
{

SetCapabilityBit::SetCapabilityBit(unsigned char bitNumber, bool enabled) :
	Message(MessageCode::SET_CAPABILITY_BIT_REQUEST)
{
	m_msg.header.msgType1 = INTERMEDIATE_MSG_TYP1;
	m_msg.header.msgType2 = INTERMEDIATE_MSG_TYP2;
	m_msg.opCode1 = 0x01;
	m_msg.opCode2 = 0x03;
	m_msg.bitNumber = bitNumber;
	m_msg.setOrUnset = enabled ? 0x01 : 0x00;
	m_msg.setLength();
	m_msg.setChecksum();
}

SetCapabilityBit::~SetCapabilityBit()
{
}

int SetCapabilityBit::getNumBytes()
{
	return sizeof(m_msg);
}

unsigned char *SetCapabilityBit::getBuffer()
{
	return (unsigned char *)&m_msg;
}

} /* namespace cea2045 */
