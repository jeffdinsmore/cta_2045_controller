#ifndef INTERMEDIATE_RESPONSE_CODE_H_
#define INTERMEDIATE_RESPONSE_CODE_H_

inline const char* intermediateResponseCodeName(unsigned char code)
{
	switch (code)
	{
	case 0x00: return "success";
	case 0x01: return "command_not_implemented";
	case 0x02: return "bad_value";
	case 0x03: return "command_length_error";
	case 0x04: return "response_length_error";
	case 0x05: return "busy";
	case 0x06: return "other_error";
	case 0x07: return "customer_override";
	case 0x08: return "command_not_enabled";
	default: return "reserved";
	}
}

#endif
