/*
 * reference:  https://github.com/epri-dev/CTA-2045-UCM-CPP-Library.git
 * UCMImpl.cpp
 *
 *  Created on: Aug 26, 2015
 *      Original Author: dupes
 *      Modifying Author: Midrar Adham
 */

#include "UCMImpl.h"
#include <easylogging++.h>

#include <cea2045/util/MSTimer.h>

#include <chrono>

#include <iostream>

#include <fstream>


using namespace std;

namespace
{
const char* messageCodeName(cea2045::MessageCode code)
{
	switch (code)
	{
	case cea2045::MessageCode::NONE:
		return "NONE";
	case cea2045::MessageCode::MAX_PAYLOAD_REQUEST:
		return "MAX_PAYLOAD_REQUEST";
	case cea2045::MessageCode::MAX_PAYLOAD_RESPONSE:
		return "MAX_PAYLOAD_RESPONSE";
	case cea2045::MessageCode::SUPPORT_DATALINK_MESSAGES:
		return "SUPPORT_DATALINK_MESSAGES";
	case cea2045::MessageCode::SUPPORT_INTERMEDIATE_MESSAGES:
		return "SUPPORT_INTERMEDIATE_MESSAGES";
	case cea2045::MessageCode::BASIC_CRITICAL_PEAK_EVENT_REQUEST:
		return "BASIC_CRITICAL_PEAK_EVENT_REQUEST";
	case cea2045::MessageCode::BASIC_END_SHED_REQUEST:
		return "BASIC_END_SHED_REQUEST";
	case cea2045::MessageCode::BASIC_SHED_REQUEST:
		return "BASIC_SHED_REQUEST";
	case cea2045::MessageCode::BASIC_GRID_EMERGENCY_REQUEST:
		return "BASIC_GRID_EMERGENCY_REQUEST";
	case cea2045::MessageCode::BASIC_LOAD_UP_REQUEST:
		return "BASIC_LOAD_UP_REQUEST";
	case cea2045::MessageCode::BASIC_OUTSIDE_COMM_CONNECTION_STATUS_MESSAGE:
		return "BASIC_OUTSIDE_COMM_CONNECTION_STATUS_MESSAGE";
	case cea2045::MessageCode::BASIC_PRESENT_RELATIVE_PRICE_REQUEST:
		return "BASIC_PRESENT_RELATIVE_PRICE_REQUEST";
	case cea2045::MessageCode::BASIC_NEXT_RELATIVE_PRICE_REQUEST:
		return "BASIC_NEXT_RELATIVE_PRICE_REQUEST";
	case cea2045::MessageCode::BASIC_QUERY_OPERATIONAL_STATE_REQUEST:
		return "BASIC_QUERY_OPERATIONAL_STATE_REQUEST";
	case cea2045::MessageCode::BASIC_POWER_LEVEL:
		return "BASIC_POWER_LEVEL";
	case cea2045::MessageCode::DEVICE_INFORMATION_REQUEST:
		return "DEVICE_INFORMATION_REQUEST";
	case cea2045::MessageCode::GET_COMMODITY_REQUEST:
		return "GET_COMMODITY_REQUEST";
	case cea2045::MessageCode::GET_TEMPERATURE_OFFSET:
		return "GET_TEMPERATURE_OFFSET";
	case cea2045::MessageCode::GET_SETPOINTS_REQUEST:
		return "GET_SETPOINTS_REQUEST";
	case cea2045::MessageCode::GET_PRESENT_TEMPERATURE_REQUEST:
		return "GET_PRESENT_TEMPERATURE_REQUEST";
	case cea2045::MessageCode::SET_TEMPERATURE_OFFSET_REQUEST:
		return "SET_TEMPERATURE_OFFSET_REQUEST";
	case cea2045::MessageCode::SET_SETPOINTS_REQUEST:
		return "SET_SETPOINTS_REQUEST";
	case cea2045::MessageCode::SET_ENERGY_PRICE_REQUEST:
		return "SET_ENERGY_PRICE_REQUEST";
	case cea2045::MessageCode::START_CYCLING_REQUEST:
		return "START_CYCLING_REQUEST";
	case cea2045::MessageCode::TERMINATE_CYCLING_REQUEST:
		return "TERMINATE_CYCLING_REQUEST";
	case cea2045::MessageCode::CUSTOMER_OVERRIDE_RESPONSE:
		return "CUSTOMER_OVERRIDE_RESPONSE";
	}

	return "UNKNOWN_MESSAGE";
}

const char* linkNakReasonName(cea2045::LinkLayerNakCode reason)
{
	switch (reason)
	{
	case cea2045::LinkLayerNakCode::NO_REASON:
		return "No reason";
	case cea2045::LinkLayerNakCode::INVALID_BYTE:
		return "Invalid byte";
	case cea2045::LinkLayerNakCode::INVALID_LENGTH:
		return "Invalid length";
	case cea2045::LinkLayerNakCode::CHECKSUM_ERROR:
		return "Checksum error";
	case cea2045::LinkLayerNakCode::RESERVED:
		return "Reserved";
	case cea2045::LinkLayerNakCode::MESSAGE_TIMEOUT:
		return "Message timeout";
	case cea2045::LinkLayerNakCode::UNSUPPORTED_MESSAGE_TYPE:
		return "Unsupported message type";
	case cea2045::LinkLayerNakCode::REQUEST_NOT_SUPPORTED:
		return "Request not supported";
	case cea2045::LinkLayerNakCode::NONE:
		return "Unknown link NAK reason";
	}

	return "Unknown link NAK reason";
}

const char* appNakReasonName(unsigned char reason)
{
	switch (reason)
	{
	case 0x00:
		return "No reason given";
	case 0x01:
		return "Opcode1 not supported";
	case 0x02:
		return "Opcode2 invalid";
	case 0x03:
		return "Busy";
	case 0x04:
		return "Invalid message length";
	case 0x05:
		return "Customer override is in effect";
	default:
		return "Reserved or unknown application NAK reason";
	}
}
}

UCMImpl::UCMImpl()
{
	m_sgdMaxPayload = cea2045::MaxPayloadLengthCode::LENGTH2;
}

//======================================================================================

UCMImpl::~UCMImpl()
{
}

//======================================================================================

bool UCMImpl::isMessageTypeSupported(cea2045::MessageTypeCode messageType)
{
	LOG(INFO) << "message type supported received: " << (int)messageType;

	if (messageType == cea2045::MessageTypeCode::NONE)
		return false;

	return true;
}

//======================================================================================

cea2045::MaxPayloadLengthCode UCMImpl::getMaxPayload()
{
	LOG(INFO) << "max payload request received";

	return cea2045::MaxPayloadLengthCode::LENGTH4096;
}

//======================================================================================

void UCMImpl::processMaxPayloadResponse(cea2045::MaxPayloadLengthCode maxPayload)
{
	LOG(INFO) << "max payload response received";

	m_sgdMaxPayload = maxPayload;
}

//======================================================================================

void UCMImpl::processDeviceInfoResponse(cea2045::cea2045DeviceInfoResponse* message)
{
	LOG(INFO) << "device info response received";

	LOG(INFO) << "    device type: " << message->getDeviceType();
	LOG(INFO) << "      vendor ID: " << message->getVendorID();

	LOG(INFO) << "  firmware date: "
			<< 2000 + (int)message->firmwareYear20xx << "-" << (int)message->firmwareMonth << "-" << (int)message->firmwareDay;
}

//======================================================================================

void UCMImpl::processCommodityResponse(cea2045::cea2045CommodityResponse* message)
{
	LOG(INFO) << "commodity response received.  count: " << message->getCommodityDataCount();
    time_t t = time(0);
    // opeen file
    ofstream out;
    out.open("log.csv",ios_base::out|ios_base::app);

	int count = message->getCommodityDataCount();


        string ti = asctime((localtime(&t)));
        ti.pop_back();
	out<<ti<<", ";
	for (int x = 0; x < count; x++)
	{
		cea2045::cea2045CommodityData *data = message->getCommodityData(x);

		LOG(INFO) << "commodity data: " << x;
		LOG(INFO) << "        code: " << (int)data->commodityCode;
		LOG(INFO) << "  cumulative: " << data->getCumulativeAmount();
		LOG(INFO) << "   inst rate: " << data->getInstantaneousRate();
		out<<(int)data->commodityCode<<", ";
		out<<data->getCumulativeAmount()<<", "<<data->getInstantaneousRate()<<", ";
	}
}

//======================================================================================

void UCMImpl::processAckReceived(cea2045::MessageCode messageCode)
{
	LOG(INFO) << "link ACK received for " << messageCodeName(messageCode);

	switch (messageCode)
	{

	case cea2045::MessageCode::SUPPORT_DATALINK_MESSAGES:
		LOG(INFO) << "supports data link messages";
		break;

	case cea2045::MessageCode::SUPPORT_INTERMEDIATE_MESSAGES:
		LOG(INFO) << "supports intermediate messages";
		break;

	default:
		break;
	}
}

//======================================================================================

void UCMImpl::processNakReceived(cea2045::LinkLayerNakCode nak, cea2045::MessageCode messageCode)
{
	LOG(WARNING) << "link NAK received for " << messageCodeName(messageCode)
			 << ". Reason: " << linkNakReasonName(nak)
			 << " (0x" << std::hex << static_cast<int>(nak) << std::dec << ")";

	if (nak == cea2045::LinkLayerNakCode::UNSUPPORTED_MESSAGE_TYPE)
	{
		switch (messageCode)
		{

		case cea2045::MessageCode::SUPPORT_DATALINK_MESSAGES:
			LOG(WARNING) << "does not support data link";
			break;

		case cea2045::MessageCode::SUPPORT_INTERMEDIATE_MESSAGES:
			LOG(WARNING) << "does not support intermediate";
			break;

		default:
			break;
		}
	}
}

//======================================================================================

void UCMImpl::processOperationalStateReceived(cea2045::cea2045Basic *message)
{
    ofstream out;
    out.open("log.csv",ios_base::out|ios_base::app);
	LOG(INFO) << "operational state received: " << (int)message->opCode2;
    out<<(int)message->opCode2<<"\n";
	cout << "\nPress Enter for a list of commands\n";
}

//======================================================================================

void UCMImpl::processAppAckReceived(cea2045::cea2045Basic* message)
{
	LOG(INFO) << "app ack received";
}

//======================================================================================

void UCMImpl::processAppNakReceived(cea2045::cea2045Basic* message)
{
	LOG(WARNING) << "application NAK received. Reason: "
			 << appNakReasonName(message->opCode2)
			 << " (0x" << std::hex << static_cast<int>(message->opCode2) << std::dec << ")";
}

//======================================================================================

void UCMImpl::processAppCustomerOverride(cea2045::cea2045Basic* message)
{
	LOG(INFO) << "app cust override received: " << (int)message->opCode2;
}

//======================================================================================

void UCMImpl::processIncompleteMessage(const unsigned char *buffer, unsigned int numBytes)
{
	LOG(WARNING) << "incomplete message received: " << numBytes;
}
