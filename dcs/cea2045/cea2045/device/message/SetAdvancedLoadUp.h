#ifndef CEA2045_SETADVANCEDLOADUP_H_
#define CEA2045_SETADVANCEDLOADUP_H_

#include "Message.h"
#include "../../util/Checksum.h"
#include "../../message/CEA2045Message.h"

namespace cea2045 {

class SetAdvancedLoadUp : public Message {
private:
    struct MandatoryFields {
        struct cea2045MessageHeader header;
        unsigned char opCode1;
        unsigned char opCode2;
        unsigned short duration;
        unsigned short value;
        unsigned char units;
    } __attribute__((packed));

    unsigned char m_buffer[sizeof(MandatoryFields) + 1 + sizeof(unsigned short)];
    int m_numBytes;

    void initialize(
        unsigned short duration,
        unsigned short value,
        unsigned char units,
        bool hasEfficiency,
        unsigned char efficiency);

public:
    SetAdvancedLoadUp(unsigned short duration, unsigned short value, unsigned char units);
    SetAdvancedLoadUp(
        unsigned short duration,
        unsigned short value,
        unsigned char units,
        unsigned char efficiency);
    virtual ~SetAdvancedLoadUp();

    virtual int getNumBytes();
    virtual unsigned char *getBuffer();
};

} /* namespace cea2045 */

#endif /* CEA2045_SETADVANCEDLOADUP_H_ */
