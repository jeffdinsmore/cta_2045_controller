/* Read and print one CTA-2045 Commodity response without saving any data. */

#include <cta2045/communicationport/CEA2045SerialPort.h>
#include <cta2045/device/DeviceFactory.h>
#include <cta2045/processmessage/IUCM.h>

#include <easylogging++.h>

#include <condition_variable>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>

using namespace cea2045;

INITIALIZE_EASYLOGGINGPP

namespace {

const char* commodityCodeName(unsigned char code)
{
	switch (code)
	{
	case 0: return "Electricity consumed (W & W-hr)";
	case 1: return "Electricity produced (W & W-hr)";
	case 2: return "Natural gas (English units)";
	case 3: return "Water (English units)";
	case 4: return "Natural gas (metric units)";
	case 5: return "Water (metric units)";
	case 6: return "Total energy storage/take capacity (W-hr)";
	case 7: return "Present energy storage/take capacity (W-hr)";
	case 8: return "Rated maximum consumption level (W)";
	case 9: return "Rated maximum production level (W)";
	case 10: return "Advanced load up total energy storage/take capacity (W-hr)";
	case 11: return "Advanced load up present energy storage/take capacity (W-hr)";
	default: return "Reserved commodity code";
	}
}

bool commodityUsesInstantaneousRate(unsigned char code)
{
	return code != 6 && code != 7 && code != 10 && code != 11;
}

const char* responseCodeName(ResponseCode code)
{
	switch (code)
	{
	case ResponseCode::OK: return "ok";
	case ResponseCode::TIMEOUT: return "timeout";
	case ResponseCode::BAD_CRC: return "bad CRC";
	case ResponseCode::INVALID_RESPONSE: return "invalid response";
	case ResponseCode::NO_ACK_RECEIVED: return "no ACK received";
	case ResponseCode::NAK: return "NAK";
	}
	return "unknown";
}

const char* messageCodeName(MessageCode code)
{
	switch (code)
	{
	case MessageCode::GET_COMMODITY_REQUEST: return "Get Commodity";
	case MessageCode::BASIC_QUERY_OPERATIONAL_STATE_REQUEST:
		return "Query Operational State";
	case MessageCode::DEVICE_INFORMATION_REQUEST: return "Device Information";
	case MessageCode::SUPPORT_DATALINK_MESSAGES: return "Data-link support query";
	case MessageCode::SUPPORT_INTERMEDIATE_MESSAGES:
		return "Intermediate-message support query";
	default: return "CTA-2045 message";
	}
}

const char* linkNakReasonName(LinkLayerNakCode code)
{
	switch (code)
	{
	case LinkLayerNakCode::NO_REASON: return "No reason";
	case LinkLayerNakCode::INVALID_BYTE: return "Invalid byte";
	case LinkLayerNakCode::INVALID_LENGTH: return "Invalid length";
	case LinkLayerNakCode::CHECKSUM_ERROR: return "Checksum error";
	case LinkLayerNakCode::RESERVED: return "Reserved";
	case LinkLayerNakCode::MESSAGE_TIMEOUT: return "Message timeout";
	case LinkLayerNakCode::UNSUPPORTED_MESSAGE_TYPE: return "Unsupported message type";
	case LinkLayerNakCode::REQUEST_NOT_SUPPORTED: return "Request not supported";
	case LinkLayerNakCode::NONE: return "Unknown reason";
	}
	return "Unknown reason";
}

const char* applicationNakReasonName(unsigned char code)
{
	switch (code)
	{
	case 0x00: return "No reason given";
	case 0x01: return "Opcode1 not supported";
	case 0x02: return "Opcode2 invalid";
	case 0x03: return "Busy";
	case 0x04: return "Invalid message length";
	case 0x05: return "Customer override is in effect";
	default: return "Reserved or unknown reason";
	}
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

class CommodityDiagnosticUCM : public IUCM
{
public:
	CommodityDiagnosticUCM()
		: m_commodityReceived(false), m_commoditySuccess(false),
		  m_operationalStateReceived(false) {}

	bool waitForCommodity(unsigned int timeoutSeconds)
	{
		std::unique_lock<std::mutex> lock(m_mutex);
		return m_condition.wait_for(
			lock,
			std::chrono::seconds(timeoutSeconds),
			[this] { return m_commodityReceived; });
	}

	bool commoditySucceeded() const
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		return m_commoditySuccess;
	}

	bool waitForOperationalState(unsigned int timeoutSeconds)
	{
		std::unique_lock<std::mutex> lock(m_mutex);
		return m_condition.wait_for(
			lock,
			std::chrono::seconds(timeoutSeconds),
			[this] { return m_operationalStateReceived; });
	}

	bool isMessageTypeSupported(MessageTypeCode messageType) override
	{
		return messageType != MessageTypeCode::NONE;
	}

	MaxPayloadLengthCode getMaxPayload() override
	{
		return MaxPayloadLengthCode::LENGTH2;
	}

	void processCommodityResponse(cea2045CommodityResponse* message) override
	{
		const bool success = message->responseCode == 0x00;
		if (!success)
		{
			std::cerr << "Commodity response rejected; intermediate response code: "
				<< static_cast<unsigned int>(message->responseCode) << '\n';
		}
		else
		{
			const int count = message->getCommodityDataCount();
			std::cout << "Commodity response received (" << count
				<< (count == 1 ? " record)\n" : " records)\n");
			for (int index = 0; index < count; ++index)
			{
				cea2045CommodityData* data = message->getCommodityData(index);
				const unsigned char rawCode = data->commodityCode;
				const unsigned char code = rawCode & 0x7F;
				const bool measured = (rawCode & 0x80) != 0;

				std::cout << "\nCommodity code: " << static_cast<unsigned int>(code)
					<< " - " << commodityCodeName(code) << '\n'
					<< "Source: " << (measured ? "Measured" : "Estimated") << '\n'
					<< "Cumulative: " << data->getCumulativeAmount() << '\n';
				if (commodityUsesInstantaneousRate(code))
					std::cout << "Instantaneous rate: "
						<< data->getInstantaneousRate() << '\n';
				else
					std::cout << "Instantaneous rate: not used\n";
			}
		}

		{
			std::lock_guard<std::mutex> lock(m_mutex);
			m_commoditySuccess = success;
			m_commodityReceived = true;
		}
		m_condition.notify_one();
	}

	void processMaxPayloadResponse(MaxPayloadLengthCode) override {}
	void processDeviceInfoResponse(cea2045DeviceInfoResponse*) override {}
	void processSetEnergyPriceResponse(cea2045IntermediateResponse*) override {}
	void processSetTemperatureOffsetResponse(cea2045IntermediateResponse*) override {}
	void processGetTemperatureOffsetResponse(cea2045GetTemperateOffsetResponse*) override {}
	void processGetSetpointsResponse(cea2045GetSetpointsResponse2*) override {}
	void processGetSetpointsResponse(cea2045GetSetpointsResponse1*) override {}
	void processSetSetpointsResponse(cea2045IntermediateResponse*) override {}
	void processStartCyclingResponse(cea2045IntermediateResponse*) override {}
	void processTerminateCyclingResponse(cea2045IntermediateResponse*) override {}
	void processSetAdvancedLoadUpResponse(cea2045IntermediateResponse*) override {}
	void processGetAdvancedLoadUpResponse(
		cea2045GetAdvancedLoadUpResponse*, unsigned short) override {}
	void processGetPresentTemperatureResponse(
		cea2045GetPresentTemperatureResponse*) override {}
	void processGetUTCTimeResponse(cea2045GetUTCTimeResponse*) override {}
	void processAckReceived(MessageCode messageCode) override
	{
		std::cout << "Link ACK received: " << messageCodeName(messageCode) << '\n';
	}

	void processNakReceived(LinkLayerNakCode nak, MessageCode messageCode) override
	{
		std::cerr << "Link NAK received: " << messageCodeName(messageCode)
			<< " - " << linkNakReasonName(nak)
			<< " (code " << static_cast<unsigned int>(nak) << ")\n";
	}

	void processAppAckReceived(cea2045Basic* message) override
	{
		std::cout << "Application ACK received"
			<< " (Opcode1=" << static_cast<unsigned int>(message->opCode1)
			<< ", Opcode2=" << static_cast<unsigned int>(message->opCode2)
			<< ")\n";
	}

	void processAppNakReceived(cea2045Basic* message) override
	{
		std::cerr << "Application NAK received: "
			<< applicationNakReasonName(message->opCode2)
			<< " (code " << static_cast<unsigned int>(message->opCode2) << ")\n";
	}

	void processOperationalStateReceived(cea2045Basic* message) override
	{
		std::cout << "\nOperational state: "
			<< static_cast<unsigned int>(message->opCode2)
			<< " - " << operationalStateName(message->opCode2) << '\n';
		{
			std::lock_guard<std::mutex> lock(m_mutex);
			m_operationalStateReceived = true;
		}
		m_condition.notify_one();
	}

	void processAppCustomerOverride(cea2045Basic* message) override
	{
		std::cout << "Customer override received"
			<< " (code " << static_cast<unsigned int>(message->opCode2) << ")\n";
	}

	void processIncompleteMessage(
		const unsigned char*, unsigned int numBytes) override
	{
		std::cerr << "Incomplete CTA-2045 message received: "
			<< numBytes << " bytes\n";
	}

private:
	mutable std::mutex m_mutex;
	std::condition_variable m_condition;
	bool m_commodityReceived;
	bool m_commoditySuccess;
	bool m_operationalStateReceived;
};

void disableLibraryLogging()
{
	el::Configurations configuration;
	configuration.setToDefault();
	configuration.setGlobally(el::ConfigurationType::ToFile, "false");
	configuration.setGlobally(el::ConfigurationType::ToStandardOutput, "false");
	el::Loggers::reconfigureAllLoggers(configuration);
}

void printUsage(const char* program)
{
	std::cout << "Usage: " << program << " [serial-port]\n"
		<< "Read and print one CTA-2045 Commodity response.\n"
		<< "Default serial port: /dev/ttyUSB0\n";
}

} // namespace

int main(int argc, char* argv[])
{
	if (argc > 2 || (argc == 2
			&& (std::string(argv[1]) == "-h" || std::string(argv[1]) == "--help")))
	{
		printUsage(argv[0]);
		return argc > 2 ? EXIT_FAILURE : EXIT_SUCCESS;
	}

	disableLibraryLogging();
	const std::string serialPort = argc == 2 ? argv[1] : "/dev/ttyUSB0";
	CEA2045SerialPort port(serialPort);
	if (!port.open())
	{
		std::cerr << "Unable to open serial port: " << serialPort << '\n';
		return EXIT_FAILURE;
	}

	CommodityDiagnosticUCM ucm;
	std::unique_ptr<ICEA2045DeviceUCM> device(DeviceFactory::createUCM(&port, &ucm));
	if (!device->start())
	{
		std::cerr << "Unable to start CTA-2045 communication\n";
		port.close();
		return EXIT_FAILURE;
	}

	std::cout << "Requesting one Commodity read from " << serialPort << "...\n";
	ResponseCodes result = device->intermediateGetCommodity().get();
	if (result.responesCode != ResponseCode::OK)
	{
		std::cerr << "Commodity request failed: "
			<< responseCodeName(result.responesCode) << '\n';
		device->shutDown();
		port.close();
		return EXIT_FAILURE;
	}

	if (!ucm.waitForCommodity(2))
	{
		std::cerr << "Commodity request completed without a Commodity response\n";
		device->shutDown();
		port.close();
		return EXIT_FAILURE;
	}

	const bool commoditySuccess = ucm.commoditySucceeded();
	if (!commoditySuccess)
	{
		device->shutDown();
		port.close();
		return EXIT_FAILURE;
	}

	std::cout << "\nRequesting operational state...\n";
	ResponseCodes stateResult = device->basicQueryOperationalState().get();
	if (stateResult.responesCode != ResponseCode::OK)
	{
		std::cerr << "Operational-state request failed: "
			<< responseCodeName(stateResult.responesCode) << '\n';
		device->shutDown();
		port.close();
		return EXIT_FAILURE;
	}
	if (!ucm.waitForOperationalState(2))
	{
		std::cerr << "Operational-state request completed without a response\n";
		device->shutDown();
		port.close();
		return EXIT_FAILURE;
	}

	device->shutDown();
	port.close();
	return EXIT_SUCCESS;
}
