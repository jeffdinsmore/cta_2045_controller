#include "SetAdvancedLoadUp.h"

#include <cstring>

namespace cea2045 {

SetAdvancedLoadUp::SetAdvancedLoadUp(unsigned short duration, unsigned short value, 
                                   unsigned char units) :
    Message(MessageCode::ADVANCED_LOAD_UP_REQUEST)
{
    initialize(duration, value, units, false, 0);
}

SetAdvancedLoadUp::SetAdvancedLoadUp(
    unsigned short duration,
    unsigned short value,
    unsigned char units,
    unsigned char efficiency) :
    Message(MessageCode::ADVANCED_LOAD_UP_REQUEST)
{
    initialize(duration, value, units, true, efficiency);
}

void SetAdvancedLoadUp::initialize(
    unsigned short duration,
    unsigned short value,
    unsigned char units,
    bool hasEfficiency,
    unsigned char efficiency)
{
    std::cout << "Creating Advanced Load Up message..." << std::endl;
    std::cout << "Setting message fields:" << std::endl
              << "Duration: " << duration << std::endl
              << "Value: " << value << std::endl
              << "Units: " << (int)units << std::endl;
    if (hasEfficiency)
        std::cout << "Suggested efficiency: " << (int)efficiency << std::endl;

    MandatoryFields fields;
    std::memset(&fields, 0, sizeof(fields));
    fields.header.msgType1 = INTERMEDIATE_MSG_TYP1;
    fields.header.msgType2 = INTERMEDIATE_MSG_TYP2;
    fields.header.length = htobe16(hasEfficiency ? 8 : 7);
    fields.opCode1 = 0x0C;
    fields.opCode2 = 0x00;
    fields.duration = htobe16(duration);
    fields.value = htobe16(value);
    fields.units = units;

    std::memcpy(m_buffer, &fields, sizeof(fields));
    int checksumOffset = sizeof(fields);
    if (hasEfficiency)
        m_buffer[checksumOffset++] = efficiency;
    const unsigned short checksum = Checksum::calculate(m_buffer, checksumOffset);
    std::memcpy(m_buffer + checksumOffset, &checksum, sizeof(checksum));
    m_numBytes = checksumOffset + sizeof(checksum);
}

SetAdvancedLoadUp::~SetAdvancedLoadUp()
{
}

int SetAdvancedLoadUp::getNumBytes()
{
    return m_numBytes;
}

unsigned char* SetAdvancedLoadUp::getBuffer()
{
    return m_buffer;
}

} /* namespace cea2045 */
