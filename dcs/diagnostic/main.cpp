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

class CommodityDiagnosticUCM : public IUCM
{
public:
	CommodityDiagnosticUCM() : m_received(false), m_success(false) {}

	bool waitForCommodity(unsigned int timeoutSeconds)
	{
		std::unique_lock<std::mutex> lock(m_mutex);
		return m_condition.wait_for(
			lock,
			std::chrono::seconds(timeoutSeconds),
			[this] { return m_received; });
	}

	bool succeeded() const
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		return m_success;
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
			m_success = success;
			m_received = true;
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
	void processAckReceived(MessageCode) override {}
	void processNakReceived(LinkLayerNakCode, MessageCode) override {}
	void processAppAckReceived(cea2045Basic*) override {}
	void processAppNakReceived(cea2045Basic*) override {}
	void processOperationalStateReceived(cea2045Basic*) override {}
	void processAppCustomerOverride(cea2045Basic*) override {}
	void processIncompleteMessage(const unsigned char*, unsigned int) override {}

private:
	mutable std::mutex m_mutex;
	std::condition_variable m_condition;
	bool m_received;
	bool m_success;
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

	const bool success = ucm.succeeded();
	device->shutDown();
	port.close();
	return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
