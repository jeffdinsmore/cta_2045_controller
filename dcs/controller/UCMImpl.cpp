/*
 * reference:  https://github.com/epri-dev/CTA-2045-UCM-CPP-Library.git
 * UCMImpl.cpp
 *
 *  Created on: Aug 26, 2015
 *      Original Author: dupes
 *      Modifying Author: Midrar Adham
 */

#include "UCMImpl.h"
#include "CtaEventLog.h"
#include "IntermediateResponseCode.h"
#include <easylogging++.h>

#include <cea2045/util/MSTimer.h>

#include <chrono>
#include <cstdlib>
#include <iomanip>

#include <iostream>

#include <fstream>
#include <sstream>

#include <sys/stat.h>

using namespace std;

namespace
{
const char* LOG_DIRECTORY = "logs";
const char* COMMODITY_CSV_HEADER =
	"timestamp_pacific,response_code,response_name,"
	"commodity_code_1,cumulative_Wh_1,insta_rate_W_1,,"
	"commodity_code_2,cumulative_Wh_2,insta_rate_W_2,,"
	"commodity_code_3,cumulative_Wh_3,insta_rate_W_3,"
	"operational_state";

const char* commodityLogPath()
{
	const char* configured = std::getenv("CTA_COMMODITY_LOG_PATH");
	return configured != NULL && configured[0] != '\0'
		? configured
		: "logs/log.csv";
}

const char* deviceInfoLogPath()
{
	const char* configured = std::getenv("CTA_DEVICE_INFO_LOG_PATH");
	return configured != NULL && configured[0] != '\0'
		? configured
		: "logs/cta_device_information.csv";
}

string csvEscape(const string& value)
{
	if (value.find_first_of(",\"\r\n") == string::npos)
		return value;

	string escaped = "\"";
	for (string::const_iterator character = value.begin();
			character != value.end(); ++character)
	{
		escaped += *character == '"' ? "\"\"" : string(1, *character);
	}
	escaped += '"';
	return escaped;
}

string bytesToHex(const unsigned char *bytes, size_t count)
{
	ostringstream value;
	value << uppercase << hex << setfill('0');
	for (size_t index = 0; index < count; index++)
		value << setw(2) << static_cast<unsigned int>(bytes[index]);
	return value.str();
}

bool bytesAreAllZero(const unsigned char *bytes, size_t count)
{
	for (size_t index = 0; index < count; index++)
	{
		if (bytes[index] != 0)
			return false;
	}
	return true;
}

string fixedAscii(const unsigned char *bytes, size_t count)
{
	size_t end = 0;
	while (end < count && bytes[end] != 0)
		end++;
	while (end > 0 && bytes[end - 1] == ' ')
		end--;
	return string(reinterpret_cast<const char *>(bytes), end);
}

string redactSerial(const string& serial)
{
	if (serial.empty())
		return "";
	const size_t visible = serial.size() < 6 ? serial.size() / 2 : 6;
	return serial.substr(0, visible) + string(serial.size() - visible, '*');
}

const char* deviceTypeName(unsigned short deviceType)
{
	switch (deviceType)
	{
	case 0x0002: return "Water Heater - Electric";
	case 0x0003: return "Water Heater - Heat Pump";
	case 0x001B: return "Water Heater - Heat Pump Variable Capacity/Speed";
	case 0x001C: return "Water Heater - Phase Change Material";
	default: return "Unknown";
	}
}

void ensureLogDirectoryExists()
{
	mkdir(LOG_DIRECTORY, 0755);
}

string currentDateTime()
{
	time_t now = time(NULL);
	struct tm localTime;
	localtime_r(&now, &localTime);

	char timestamp[20];
	strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", &localTime);
	return timestamp;
}

const char* messageCodeName(cea2045::MessageCode code)
{
	switch (code)
	{
	case cea2045::MessageCode::NONE:
		return "none";
	case cea2045::MessageCode::MAX_PAYLOAD_REQUEST:
		return "max_payload_request";
	case cea2045::MessageCode::MAX_PAYLOAD_RESPONSE:
		return "max_payload_response";
	case cea2045::MessageCode::SUPPORT_DATALINK_MESSAGES:
		return "support_datalink_messages";
	case cea2045::MessageCode::SUPPORT_INTERMEDIATE_MESSAGES:
		return "support_intermediate_messages";
	case cea2045::MessageCode::ADVANCED_LOAD_UP_REQUEST:
		return "advanced_load_up";
	case cea2045::MessageCode::SET_CAPABILITY_BIT_REQUEST:
		return "set_capability_bit";
	case cea2045::MessageCode::BASIC_CRITICAL_PEAK_EVENT_REQUEST:
		return "critical_peak";
	case cea2045::MessageCode::BASIC_END_SHED_REQUEST:
		return "run_normal";
	case cea2045::MessageCode::BASIC_SHED_REQUEST:
		return "shed";
	case cea2045::MessageCode::BASIC_GRID_EMERGENCY_REQUEST:
		return "grid_emergency";
	case cea2045::MessageCode::BASIC_LOAD_UP_REQUEST:
		return "load_up";
	case cea2045::MessageCode::BASIC_OUTSIDE_COMM_CONNECTION_STATUS_MESSAGE:
		return "outside_communication";
	case cea2045::MessageCode::BASIC_PRESENT_RELATIVE_PRICE_REQUEST:
		return "present_relative_price";
	case cea2045::MessageCode::BASIC_NEXT_RELATIVE_PRICE_REQUEST:
		return "next_relative_price";
	case cea2045::MessageCode::BASIC_QUERY_OPERATIONAL_STATE_REQUEST:
		return "query_operational_state";
	case cea2045::MessageCode::BASIC_POWER_LEVEL:
		return "power_level";
	case cea2045::MessageCode::DEVICE_INFORMATION_REQUEST:
		return "device_information";
	case cea2045::MessageCode::GET_COMMODITY_REQUEST:
		return "get_commodity";
	case cea2045::MessageCode::GET_TEMPERATURE_OFFSET:
		return "get_temperature_offset";
	case cea2045::MessageCode::GET_SETPOINTS_REQUEST:
		return "get_setpoints";
	case cea2045::MessageCode::GET_PRESENT_TEMPERATURE_REQUEST:
		return "get_present_temperature";
	case cea2045::MessageCode::SET_TEMPERATURE_OFFSET_REQUEST:
		return "set_temperature_offset";
	case cea2045::MessageCode::SET_SETPOINTS_REQUEST:
		return "set_setpoints";
	case cea2045::MessageCode::SET_ENERGY_PRICE_REQUEST:
		return "set_energy_price";
	case cea2045::MessageCode::START_CYCLING_REQUEST:
		return "start_cycling";
	case cea2045::MessageCode::TERMINATE_CYCLING_REQUEST:
		return "terminate_cycling";
	case cea2045::MessageCode::CUSTOMER_OVERRIDE_RESPONSE:
		return "customer_override";
	}

	return "unknown_message";
}

const char* basicOpcodeName(unsigned char opcode)
{
	switch (opcode)
	{
	case SHED: return "shed";
	case END_SHED: return "run_normal";
	case APP_ACK: return "application_ack";
	case APP_NAK: return "application_nak";
	case CPP: return "critical_peak";
	case GRID_EMERGENCY: return "grid_emergency";
	case COMM_STATUS: return "outside_communication";
	case CUST_OVERRIDE: return "customer_override";
	case OPER_STATE_REQ: return "query_operational_state";
	case OPER_STATE_RESP: return "operational_state";
	case LOADUP: return "load_up";
	default: return "basic_dr";
	}
}

std::string messageCodeOpcode1(cea2045::MessageCode code)
{
	switch (code)
	{
	case cea2045::MessageCode::ADVANCED_LOAD_UP_REQUEST: return "12";
	case cea2045::MessageCode::BASIC_CRITICAL_PEAK_EVENT_REQUEST: return "10";
	case cea2045::MessageCode::BASIC_END_SHED_REQUEST: return "2";
	case cea2045::MessageCode::BASIC_SHED_REQUEST: return "1";
	case cea2045::MessageCode::BASIC_GRID_EMERGENCY_REQUEST: return "11";
	case cea2045::MessageCode::BASIC_LOAD_UP_REQUEST: return "23";
	case cea2045::MessageCode::BASIC_OUTSIDE_COMM_CONNECTION_STATUS_MESSAGE: return "14";
	case cea2045::MessageCode::BASIC_QUERY_OPERATIONAL_STATE_REQUEST: return "18";
	case cea2045::MessageCode::GET_COMMODITY_REQUEST: return "6";
	default: return "";
	}
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

const char* commodityCodeName(unsigned char code)
{
	switch (code)
	{
	case 0:
		return "Electricity consumed (W & W-hr)";
	case 1:
		return "Electricity produced (W & W-hr)";
	case 2:
		return "Natural gas";
	case 3:
		return "Water";
	case 4:
		return "Natural gas";
	case 5:
		return "Water";
	case 6:
		return "Total energy storage/take capacity (W-hr)";
	case 7:
		return "Present energy storage/take capacity (W-hr)";
	case 8:
		return "Rated max consumption level electricity (W)";
	case 9:
		return "Rated max production level electricity (W)";
	case 10:
		return "Advanced load up total energy storage/take capacity (W-hr)";
	case 11:
		return "Advanced load up present energy storage/take capacity (W-hr)";
	default:
		return "Reserved commodity code";
	}
}

bool commodityUsesInstantaneousRate(unsigned char code)
{
	// CTA-2045 CommodityRead defines the field in every record, but storage
	// capacity commodities use only the cumulative-amount field.
	return code != 6 && code != 7 && code != 10 && code != 11;
}

bool commodityIsRecordedInCsv(unsigned char code)
{
	// Codes 8 and 9 are parsed and described above for possible future use,
	// but this test does not record rated maximum levels in its commodity CSV.
	return code != 8 && code != 9;
}

const char* operationalStateName(unsigned char code)
{
	switch (code)
	{
	case 0: return "Idle Normal";
	case 1: return "Running Normal";
	case 2: return "Running Curtailed";
	case 3: return "Running Heightened";
	case 4: return "Idle Curtailed";
	case 5: return "SGD Error Condition";
	case 6: return "Idle Heightened";
	case 7: return "Cycling on";
	case 8: return "Cycling off";
	case 9: return "Variable Following";
	case 10: return "Variable not following";
	case 11: return "Idle, opted out";
	case 12: return "Running, opted out";
	case 13: return "Running, price stream";
	case 14: return "Idle, price stream";
	default: return "Unknown operational state";
	}
}
}

UCMImpl::UCMImpl()
{
	m_sgdMaxPayload = cea2045::MaxPayloadLengthCode::LENGTH2;
	ensureLogDirectoryExists();
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
	const unsigned short payloadLength = message->getLength();
	const unsigned char responseCode = message->responseCode;
	const char* responseName = intermediateResponseCodeName(responseCode);
	const bool hasVersion = payloadLength >= 5;
	const bool hasVendorId = payloadLength >= 7;
	const bool hasDeviceType = payloadLength >= 9;
	const bool hasDeviceRevision = payloadLength >= 11;
	const bool hasCapability = payloadLength >= 15;
	const unsigned short vendorId =
			hasVendorId ? message->getVendorID() : 0;
	const unsigned short deviceType =
			hasDeviceType ? message->getDeviceType() : 0;
	const bool hasModel = payloadLength >= 32
			&& !bytesAreAllZero(message->modelNumber, sizeof(message->modelNumber));
	const bool hasSerial = payloadLength >= 48
			&& !bytesAreAllZero(message->serialNumber, sizeof(message->serialNumber));
	const string model = hasModel
			? fixedAscii(message->modelNumber, sizeof(message->modelNumber)) : "";
	const string serial = hasSerial
			? fixedAscii(message->serialNumber, sizeof(message->serialNumber)) : "";

	LOG(INFO) << "device info response received. response code: "
			  << static_cast<int>(responseCode) << " - " << responseName;
	if (hasVersion)
		LOG(INFO) << "  CTA-2045 version: "
				  << fixedAscii(message->version, sizeof(message->version));
	if (hasDeviceType)
		LOG(INFO) << "  device type: " << deviceType
				  << " - " << deviceTypeName(deviceType);
	if (hasVendorId)
		LOG(INFO) << "  vendor ID: " << vendorId;

	logCtaEvent(
		"intermediate_response", "inbound", "device_information",
		responseCode == 0x00 ? "success" : "error",
		"", "", "", "device_response",
		std::to_string(static_cast<int>(message->opCode1)),
		std::to_string(static_cast<int>(message->opCode2)),
		"", "", "", "",
		std::to_string(static_cast<int>(responseCode)), responseName);

	ofstream out(deviceInfoLogPath(), ios_base::out | ios_base::trunc);
	if (!out.is_open())
	{
		LOG(ERROR) << "failed to open device information CSV: "
				   << deviceInfoLogPath();
		return;
	}

	out << "timestamp_pacific,opcode1,opcode2,response_code,response_name,"
		   "payload_length,cta2045_version_raw_hex,cta2045_version,"
		   "cta2045_b_version_valid,vendor_id_hex,device_type_hex,"
		   "device_type_name,device_revision_hex,capability_bitmap_hex,"
		   "model_number,model_number_supported,serial_number_redacted,"
		   "serial_number_supported,firmware_year_raw,firmware_month_raw,"
		   "firmware_day_raw,firmware_date,firmware_major,firmware_minor\n";

	ostringstream vendorHex;
	vendorHex << "0x" << uppercase << hex << setw(4) << setfill('0') << vendorId;
	ostringstream deviceTypeHex;
	deviceTypeHex << "0x" << uppercase << hex << setw(4) << setfill('0') << deviceType;
	const bool validVersionB = hasVersion
			&&
			message->version[0] == 0x42 && message->version[1] == 0x00;

	out << csvEscape(currentDateTime())
		<< ',' << static_cast<unsigned int>(message->opCode1)
		<< ',' << static_cast<unsigned int>(message->opCode2)
		<< ',' << static_cast<unsigned int>(responseCode)
		<< ',' << responseName
		<< ',' << payloadLength
		<< ',';
	if (hasVersion)
		out << bytesToHex(message->version, sizeof(message->version));
	out << ',';
	if (hasVersion)
		out << csvEscape(fixedAscii(message->version, sizeof(message->version)));
	out << ',';
	if (hasVersion)
		out << (validVersionB ? "true" : "false");
	out << ',';
	if (hasVendorId)
		out << vendorHex.str();
	out << ',';
	if (hasDeviceType)
		out << deviceTypeHex.str();
	out << ',';
	if (hasDeviceType)
		out << csvEscape(deviceTypeName(deviceType));
	out << ',';
	if (hasDeviceRevision)
		out << bytesToHex(message->deviceRevision, sizeof(message->deviceRevision));
	out << ',';
	if (hasCapability)
		out << bytesToHex(message->capability, sizeof(message->capability));
	out << ',' << csvEscape(model)
		<< ',' << (hasModel ? "true" : "false")
		<< ',' << csvEscape(redactSerial(serial))
		<< ',' << (hasSerial ? "true" : "false");

	const bool hasFirmwareYear = payloadLength >= 49;
	const bool hasFirmwareMonth = payloadLength >= 50;
	const bool hasFirmwareDay = payloadLength >= 51;
	const bool hasFirmwareMajor = payloadLength >= 52;
	const bool hasFirmwareMinor = payloadLength >= 53;
	out << ',';
	if (hasFirmwareYear)
		out << static_cast<unsigned int>(message->firmwareYear20xx);
	out << ',';
	if (hasFirmwareMonth)
		out << static_cast<unsigned int>(message->firmwareMonth);
	out << ',';
	if (hasFirmwareDay)
		out << static_cast<unsigned int>(message->firmwareDay);
	out << ',';
	if (hasFirmwareYear && hasFirmwareMonth && hasFirmwareDay
			&& message->firmwareMonth <= 11
			&& message->firmwareDay >= 1 && message->firmwareDay <= 31)
	{
		ostringstream firmwareDate;
		firmwareDate << 2000 + static_cast<unsigned int>(message->firmwareYear20xx)
					 << '-' << setw(2) << setfill('0')
					 << 1 + static_cast<unsigned int>(message->firmwareMonth)
					 << '-' << setw(2)
					 << static_cast<unsigned int>(message->firmwareDay);
		out << firmwareDate.str();
	}
	out << ',';
	if (hasFirmwareMajor)
		out << static_cast<unsigned int>(message->firmwareMajor);
	out << ',';
	if (hasFirmwareMinor)
		out << static_cast<unsigned int>(message->firmwareMinor);
	out << '\n';
}

//======================================================================================

void UCMImpl::processCommodityResponse(cea2045::cea2045CommodityResponse* message)
{
	const unsigned char responseCode = message->responseCode;
	const char* responseName = intermediateResponseCodeName(responseCode);
	LOG(INFO) << "commodity response received. response code: "
			  << static_cast<int>(responseCode) << " - " << responseName
			  << "; count: " << message->getCommodityDataCount();
	logCtaEvent(
		"intermediate_response",
		"inbound",
		"get_commodity",
		responseCode == 0x00 ? "success" : "error",
		"",
		"",
		"",
		"device_response",
		"6",
		"128",
		"",
		"",
		"",
		"",
		std::to_string(static_cast<int>(responseCode)),
		responseName);
	lock_guard<mutex> logLock(m_commodityLogMutex);
	const char* csvLogPath = commodityLogPath();
	struct stat logFileInfo;
	const bool writeHeader =
		stat(csvLogPath, &logFileInfo) != 0 || logFileInfo.st_size == 0;
	ofstream out(csvLogPath, ios_base::out | ios_base::app);
	if (!out.is_open())
	{
		LOG(ERROR) << "failed to open CSV log: " << csvLogPath;
		m_commodityRowPending = false;
		return;
	}
	if (writeHeader)
		out << COMMODITY_CSV_HEADER << '\n';

	// A missing operational-state response must not cause the next commodity
	// response to be joined to an incomplete row.
	if (m_commodityRowPending)
		out << ",\n";

	const int count = message->getCommodityDataCount();
	if (count > 3)
		LOG(WARNING) << "commodity CSV supports three recorded commodity groups; received "
					 << count;
	out << currentDateTime()
		<< ',' << static_cast<int>(responseCode)
		<< ',' << responseName;
	int dataIndex = 0;
	for (int csvGroup = 0; csvGroup < 3; csvGroup++)
	{
		if (csvGroup > 0)
			out << ','; // blank separator column between commodity groups

		cea2045::cea2045CommodityData *data = nullptr;
		unsigned char commodityCode = 0;
		bool isMeasured = false;
		while (dataIndex < count)
		{
			data = message->getCommodityData(dataIndex++);
			const unsigned char rawCommodityCode = data->commodityCode;
			commodityCode = rawCommodityCode & 0x7F;
			isMeasured = (rawCommodityCode & 0x80) != 0;
			if (commodityIsRecordedInCsv(commodityCode))
				break;

			LOG(INFO) << "commodity code " << static_cast<int>(commodityCode)
					  << " - " << commodityCodeName(commodityCode)
					  << " is not recorded in the commodity CSV";
			data = nullptr;
		}

		if (data == nullptr)
		{
			out << ",,,";
			continue;
		}

		LOG(INFO) << "commodity CSV group: " << csvGroup;
		LOG(INFO) << "  commodity code: " << static_cast<int>(commodityCode)
				  << " - " << commodityCodeName(commodityCode);
		LOG(INFO) << "           source: " << (isMeasured ? "Measured" : "Estimated");
		LOG(INFO) << "  cumulative: " << data->getCumulativeAmount();
		if (commodityUsesInstantaneousRate(commodityCode))
			LOG(INFO) << "   inst rate: " << data->getInstantaneousRate();
		else
			LOG(INFO) << "   inst rate: not used for this commodity code";

		out << ',' << static_cast<int>(commodityCode)
			<< ',' << data->getCumulativeAmount()
			<< ',';
		if (commodityUsesInstantaneousRate(commodityCode))
			out << data->getInstantaneousRate();
	}
	m_commodityRowPending = true;
}

//======================================================================================

void UCMImpl::processSetAdvancedLoadUpResponse(
		cea2045::cea2045IntermediateResponse *message)
{
	const unsigned char responseCode = message->responseCode;
	const char* responseName = intermediateResponseCodeName(responseCode);
	LOG(INFO) << "advanced load up response received. response code: "
			  << static_cast<int>(responseCode) << " - " << responseName;
	logCtaEvent(
		"intermediate_response",
		"inbound",
		"advanced_load_up",
		responseCode == 0x00 ? "success" : "error",
		"",
		"",
		"",
		"device_response",
		std::to_string(static_cast<int>(message->opCode1)),
		std::to_string(static_cast<int>(message->opCode2)),
		"",
		"",
		"",
		"",
		std::to_string(static_cast<int>(responseCode)),
		responseName);
}

//======================================================================================

void UCMImpl::processAckReceived(cea2045::MessageCode messageCode)
{
	LOG(INFO) << "link ACK received: " << messageCodeName(messageCode);
	logCtaEvent(
		"link_ack",
		"inbound",
		messageCodeName(messageCode),
		"ack",
		"",
		"",
		"",
		"link_layer",
		messageCodeOpcode1(messageCode));

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
	logCtaEvent(
		"link_nak",
		"inbound",
		messageCodeName(messageCode),
		"nak",
		"",
		"",
		"",
		"link_layer",
		messageCodeOpcode1(messageCode),
		"",
		std::to_string(static_cast<int>(nak)),
		linkNakReasonName(nak));

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
	LOG(INFO) << "operational state received: " << (int)message->opCode2;
	logCtaEvent(
		"operational_state",
		"inbound",
		"query_operational_state",
		"received",
		"",
		"",
		"",
		"device_response",
		std::to_string(static_cast<int>(message->opCode1)),
		std::to_string(static_cast<int>(message->opCode2)),
		"",
		"",
		std::to_string(static_cast<int>(message->opCode2)),
		operationalStateName(message->opCode2));

	lock_guard<mutex> logLock(m_commodityLogMutex);
	if (m_commodityRowPending)
	{
		const char* csvLogPath = commodityLogPath();
		ofstream out(csvLogPath, ios_base::out | ios_base::app);
		if (!out.is_open())
		{
			LOG(ERROR) << "failed to open CSV log: " << csvLogPath;
		}
		else
		{
			out << ',' << static_cast<int>(message->opCode2) << '\n';
			m_commodityRowPending = false;
		}
	}
	cout << "\nPress Enter for a list of commands\n";
}

//======================================================================================

void UCMImpl::processAppAckReceived(cea2045::cea2045Basic* message)
{
	LOG(INFO) << "app ack received";
	logCtaEvent(
		"application_ack",
		"inbound",
		basicOpcodeName(message->opCode2),
		"ack",
		"",
		"",
		"",
		"application_layer",
		std::to_string(static_cast<int>(message->opCode1)),
		std::to_string(static_cast<int>(message->opCode2)));
}

//======================================================================================

void UCMImpl::processAppNakReceived(cea2045::cea2045Basic* message)
{
	LOG(WARNING) << "application NAK received. Reason: "
			 << appNakReasonName(message->opCode2)
				<< " (0x" << std::hex << static_cast<int>(message->opCode2) << std::dec << ")";
	logCtaEvent(
		"application_nak",
		"inbound",
		"basic_dr",
		"nak",
		"",
		"",
		"",
		"application_layer",
		std::to_string(static_cast<int>(message->opCode1)),
		std::to_string(static_cast<int>(message->opCode2)),
		std::to_string(static_cast<int>(message->opCode2)),
		appNakReasonName(message->opCode2));
}

//======================================================================================

void UCMImpl::processAppCustomerOverride(cea2045::cea2045Basic* message)
{
	LOG(INFO) << "app cust override received: " << (int)message->opCode2;
	logCtaEvent(
		"customer_override",
		"inbound",
		"customer_override",
		"received",
		"",
		"",
		"",
		"application_layer",
		std::to_string(static_cast<int>(message->opCode1)),
		std::to_string(static_cast<int>(message->opCode2)));
}

//======================================================================================

void UCMImpl::processIncompleteMessage(const unsigned char *buffer, unsigned int numBytes)
{
	LOG(WARNING) << "incomplete message received: " << numBytes;
	logCtaEvent(
		"incomplete_message",
		"inbound",
		"unknown",
		"error",
		std::to_string(numBytes),
		"",
		"",
		"link_layer");
}
