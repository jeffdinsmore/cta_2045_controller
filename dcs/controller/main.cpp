/*
 * ref: https://github.com/epri-dev/CTA-2045-UCM-CPP-Library.git
 * author: Midrar Adham
 */

#include "UCMImpl.h"
#include "CtaEventLog.h"
#include "IntermediateResponseCode.h"

#include <easylogging++.h>

#include <cta2045/device/DeviceFactory.h>

#include <cta2045/communicationport/CEA2045SerialPort.h>

#include <unistd.h>

#include <thread>
#include <chrono>

//#include <QCoreApplication>
#include <iostream>
#include <cctype>
#include <sstream>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>

using namespace cea2045;

INITIALIZE_EASYLOGGINGPP

#include <cta2045/util/MSTimer.h>

bool perform_command(char cmd, unsigned int argument, unsigned int value, unsigned int units, bool hasEfficiency, unsigned int efficiency, const string& eventId, std::shared_ptr<ICEA2045DeviceUCM> dev);
void commodity_service_loop(
	std::shared_ptr<ICEA2045DeviceUCM> dev,
	bool scheduleEnabled);
const char* responseCodeName(ResponseCode code);

void performAdvancedLoadUpReadback(
	const string& eventId,
	unsigned int delaySeconds,
	std::shared_ptr<ICEA2045DeviceUCM> dev)
{
	const string delayArgument = "delay_seconds=" + to_string(delaySeconds);
	logCtaEvent(
		"verification_sent", "outbound", "get_advanced_load_up",
		"pending", delayArgument, "", eventId, "automatic_delayed_readback",
		"12", "0");
	ResponseCodes readback = dev->intermediateGetAdvancedLoadUp().get();
	string readbackResult = responseCodeName(readback.responesCode);
	string readbackCode;
	string readbackName;
	if (readback.responesCode == ResponseCode::OK
			&& readback.hasIntermediateResponseCode)
	{
		readbackCode = to_string(
			static_cast<int>(readback.intermediateResponseCode));
		readbackName = intermediateResponseCodeName(
			readback.intermediateResponseCode);
		if (readback.intermediateResponseCode != 0x00)
			readbackResult = readbackName;
	}
	logCtaEvent(
		"verification_completed", "outbound", "get_advanced_load_up",
		readbackResult, delayArgument, "", eventId,
		"automatic_delayed_readback", "12", "0",
		"", "", "", "", readbackCode, readbackName);
}

const char* scheduledCommandName(char cmd)
{
	switch (tolower(cmd))
	{
	case 'a': return "advanced_load_up";
	case 's': return "shed";
	case 'e': return "run_normal";
	case 'l': return "load_up";
	case 'g': return "grid_emergency";
	case 'c': return "critical_peak";
	case 'o': return "outside_communication1";
	case 'v': return "get_advanced_load_up";
	default: return "unknown";
	}
}

string scheduledCommandOpcode1(char cmd)
{
	switch (tolower(cmd))
	{
	case 'a': return "12";
	case 's': return "1";
	case 'e': return "2";
	case 'c': return "10";
	case 'g': return "11";
	case 'o': return "14";
	case 'l': return "23";
	case 'v': return "12";
	default: return "";
	}
}

const char* responseCodeName(ResponseCode code)
{
	switch (code)
	{
	case ResponseCode::OK: return "ok";
	case ResponseCode::TIMEOUT: return "timeout";
	case ResponseCode::BAD_CRC: return "bad_crc";
	case ResponseCode::INVALID_RESPONSE: return "invalid_response";
	case ResponseCode::NO_ACK_RECEIVED: return "no_ack_received";
	case ResponseCode::NAK: return "nak";
	}
	return "unknown";
}

int main(int argc, char* argv[])
{
	MSTimer timer;
	bool shutdown = false;
	const bool scheduleEnabled = argc > 1 && string(argv[1]) == "--schedule";

	CEA2045SerialPort sp("/dev/ttyUSB0");
	UCMImpl ucm;
	logCtaEvent(
		"controller_started", "internal", "controller", "started",
		"", "", "", "controller");

	if (!sp.open())
	{
		LOG(ERROR) << "failed to open serial port: " << strerror(errno);
		logCtaEvent(
			"serial_open", "internal", "serial_port", "error",
			"/dev/ttyUSB0", strerror(errno), "", "controller");
		return 0;
	}
	logCtaEvent(
		"serial_open", "internal", "serial_port", "ok",
		"/dev/ttyUSB0", "", "", "controller");

	//shared_ptr<ICEA2045DeviceUCM> device = make_shared<DeviceFactory::createUCM(&sp, &ucm)>();
    //auto device = mak
	//shared_ptr<ICEA2045DeviceUCM> device = make_shared<DeviceFactory::createUCM>(&sp,&ucm);
    std::shared_ptr<ICEA2045DeviceUCM> device(DeviceFactory::createUCM(&sp,&ucm));
    //device = make_shared<ICEA2045DeviceUCM>();
    //device = DeviceFactory::createUCM(&sp,&ucm);

	device->start();
	logCtaEvent(
		"communication_started", "internal", "cta2045", "started",
		"", "", "", "controller");

	logCtaEvent(
		"query_sent", "outbound", "device_information", "pending",
		"", "", "", "startup", "1", "1");
	ResponseCodes deviceInfoResult =
		device->intermediateGetDeviceInformation().get();
	string deviceInfoCompletion =
		responseCodeName(deviceInfoResult.responesCode);
	string deviceInfoResponseCode;
	string deviceInfoResponseName;
	if (deviceInfoResult.responesCode == ResponseCode::OK
			&& deviceInfoResult.hasIntermediateResponseCode)
	{
		deviceInfoResponseCode = to_string(
			static_cast<int>(deviceInfoResult.intermediateResponseCode));
		deviceInfoResponseName = intermediateResponseCodeName(
			deviceInfoResult.intermediateResponseCode);
		if (deviceInfoResult.intermediateResponseCode != 0x00)
			deviceInfoCompletion = deviceInfoResponseName;
	}
	logCtaEvent(
		"query_completed", "outbound", "device_information",
		deviceInfoCompletion, "", "", "", "startup", "1", "1",
		"", "", "", "", deviceInfoResponseCode, deviceInfoResponseName);

	LOG(INFO) << "starting commodity service...";
	LOG(INFO) << "schedule processing is "
			  << (scheduleEnabled ? "enabled" : "disabled");
    std::thread commodity(commodity_service_loop, device, scheduleEnabled);
    commodity.detach();
    sleep(5);
	while (!shutdown)
	{
        cout<<"a- Advanced Loadup\n";
		cout<<"c- Critical Peak Event\n";
        cout<<"e- End shed/Normal\n";
        cout<<"g- Grid Emergency\n";
        cout<<"l- Loadup\n";
        cout<<"o- Outside Communication\n";
        cout<<"s- Shed\n";
		cout<<"u- Customer Override\n";
		cout<<"v- Enable advanced load up capability\n";
		cout<<"x- Disable advanced load up capability\n";
		cout<<"z- Quit and return operation to normal\n";
        cout<<"q- Quit\n";
        cout<<"enter choice: ";
		char c;
		cin >> c;

		switch (c)
		{
			case 'a':
				{
					// Values exactly matching spec example
					unsigned short duration = 60;  // 0x3C
					unsigned short value = 10;      // 5 x 100Wh = 0.5 kWh
					unsigned char units = 0x03;    // 1000Wh units
					
					std::cout << "Advanced Load Up initiated with spec values..." << std::endl;
					device->intermediateSetAdvancedLoadUp(duration, value, units).get();
					cout << "Loading..."<< endl;
					sleep(15);

					cout << "Querying operational state after CRITICAL PEAK EVENT..." << endl;
					device->basicQueryOperationalState().get();
				}
				break;

			case 'c':
				cout << "Sending CRITICAL PEAK EVENT..." << endl;
				device->basicCriticalPeakEvent(0).get();
				cout << "Loading..."<< endl;
				sleep(15);

				cout << "Querying operational state after CRITICAL PEAK EVENT..." << endl;
				device->basicQueryOperationalState().get();
   				break;

			case 'e':
				cout << "Sending END SHED..." << endl;
				device->basicEndShed(0).get();
				cout << "Loading..."<< endl;
				sleep(15);

				cout << "Querying operational state after END SHED..." << endl;
				device->basicQueryOperationalState().get();
				break;

			case 'g':
				cout << "Sending GRID EMERGENCY..." << endl;
				device->basicGridEmergency(0).get();
				cout << "Loading..."<< endl;
				sleep(15);

				cout << "Querying operational state after GRID EMERGENCY..." << endl;
				device->basicQueryOperationalState().get();
				break;

			case 'l':
				cout << "Sending LOAD UP..." << endl;
				device->basicLoadUp(0).get();
				cout << "Loading..."<< endl;
				sleep(15);

				cout << "Querying operational state after LOAD UP..." << endl;
				device->basicQueryOperationalState().get();
   				break;

			case 'o':
				cout << "Sending outside communication command..." << endl;
				device->basicOutsideCommConnectionStatus(
					OutsideCommuncatonStatusCode::Found).get();
				cout << "Loading..."<< endl;
				sleep(10);
				cout << "Querying operational state after OUTSIDE COMMUNICATION..." << endl;
				device->basicQueryOperationalState().get();
				break;

			case 'p':
				device->basicPowerLevel(63).get();		// approx 50%
				break;
			
			case 'q':
				shutdown = true;
				break;

			case 'r':
				device->basicPresentRelativePrice(101).get();	// approx twice
				break;

			case 's':
				cout << "Sending SHED..." << endl;
				device->basicShed(0).get();
				cout << "Loading..."<< endl;
				sleep(15);

				cout << "Querying operational state after SHED..." << endl;
				device->basicQueryOperationalState().get();
				break;

			case 'u':
				{
					cout << "Customer override? (0 = No, 1 = Yes): ";
					int overrideChoice = -1;
					if (!(cin >> overrideChoice) || (overrideChoice != 0 && overrideChoice != 1))
					{
						cout << "Invalid selection. Enter 0 for No or 1 for Yes." << endl;
						cin.clear();
						cin.ignore(10000, '\n');
						break;
					}
					cout << "Sending CUSTOMER OVERRIDE: "
						 << (overrideChoice == 1 ? "Yes" : "No") << "..." << endl;
					device->basicCustomerOverride(overrideChoice == 1).get();
				}
				break;
			
			case 'v':
    			cout << "Enabling Advanced Load Up capability bit 6..." << endl;
    			device->intermediateSetCapabilityBit(6, true).get();
    			break;
				
			case 'x':
    			cout << "Disabling Advanced Load Up capability bit 6..." << endl;
    			device->intermediateSetCapabilityBit(6, false).get();
    			break;
			
			case 'z':
				cout << "Returning state to normal..." << endl;
				logCtaEvent(
					"command_sent", "outbound", "run_normal", "pending", "0",
					"", "", "shutdown", "2", "0");
				{
					ResponseCodes result = device->basicEndShed(0).get();
					logCtaEvent(
						"command_completed", "outbound", "run_normal",
						responseCodeName(result.responesCode), "0",
						"", "", "shutdown", "2", "0");
				}
				cout << "Reporting outside communication disconnected..." << endl;
				logCtaEvent(
					"command_sent", "outbound", "outside_communication",
					"pending", "0", "", "", "shutdown", "14", "0");
				{
					ResponseCodes result = device->basicOutsideCommConnectionStatus(
						OutsideCommuncatonStatusCode::No).get();
					logCtaEvent(
						"command_completed", "outbound", "outside_communication",
						responseCodeName(result.responesCode), "0",
						"", "", "shutdown", "14", "0");
				}
				cout << "Loading..."<< endl;
				sleep(2);

				cout << "Querying operational state after normal command..." << endl;
				device->basicQueryOperationalState().get();
				shutdown = true;
				break;
			case 'C':
				device->intermediateGetCommodity().get();
				break;

			case 'O':
				device->intermediateGetTemperatureOffset().get();
				break;
			
			case 'S':
				device->intermediateGetSetPoint().get();
				break;

			case 'T':
				device->intermediateGetPresentTemperature().get();
				break;
			
			default:
				LOG(WARNING) << "invalid command";
				break;
		}
	}

	device->shutDown();
	logCtaEvent(
		"controller_stopped", "internal", "controller", "stopped",
		"", "", "", "controller");

	//delete (device);

	return 0;


}


bool perform_command(char cmd, unsigned int argument, unsigned int value, unsigned int units, bool hasEfficiency, unsigned int efficiency, const string& eventId, std::shared_ptr<ICEA2045DeviceUCM> dev){
	if (tolower(cmd) == 'v')
	{
		performAdvancedLoadUpReadback(eventId, argument, dev);
		return false;
	}
	const string commandName = scheduledCommandName(cmd);
	string argumentText = to_string(argument);
	if (tolower(cmd) == 'a')
		argumentText = "duration_minutes=" + to_string(argument)
			+ ";value=" + to_string(value)
			+ ";units=" + to_string(units)
			+ ";efficiency=" + (hasEfficiency ? to_string(efficiency) : "not_included");
	const string opcode1 = scheduledCommandOpcode1(cmd);
	const string opcode2 = tolower(cmd) == 'a' ? "" : to_string(argument);
	logCtaEvent(
		"command_sent", "outbound", commandName, "pending", argumentText,
		"", eventId, "schedule", opcode1, opcode2);
	ResponseCodes result;
	bool commandCompleted = true;
    switch (tolower(cmd)){
		case 'a':
			cout << "advanced load up"<< endl;
			if (hasEfficiency)
				result = dev->intermediateSetAdvancedLoadUp(
					static_cast<unsigned short>(argument),
					static_cast<unsigned short>(value),
					static_cast<unsigned char>(units),
					static_cast<unsigned char>(efficiency)).get();
			else
				result = dev->intermediateSetAdvancedLoadUp(
					static_cast<unsigned short>(argument),
					static_cast<unsigned short>(value),
					static_cast<unsigned char>(units)).get();
			break;
		case 's':
            cout<<"shedding"<<endl;
	    result = dev->basicShed(static_cast<unsigned char>(argument)).get();
            break;
        case 'e':
	    result = dev->basicEndShed(static_cast<unsigned char>(argument)).get();
            cout<<"endshedding"<<endl;
            break;
        case 'l':
            cout<<"loading up"<<endl;
	    result = dev->basicLoadUp(static_cast<unsigned char>(argument)).get();
            break;
        case 'g':
            cout<<"grid emergency"<<endl;
	    result = dev->basicGridEmergency(static_cast<unsigned char>(argument)).get();
            break;
        case 'c':
            cout<<"critical peak event"<<endl;
	    result = dev->basicCriticalPeakEvent(static_cast<unsigned char>(argument)).get();
            break;
		case 'o':
			cout<<"outside communication found"<<endl;
			result = dev->basicOutsideCommConnectionStatus(
				OutsideCommuncatonStatusCode::Found).get();
            break;

        default:
			commandCompleted = false;
			logCtaEvent(
				"command_rejected", "internal", commandName, "invalid_command",
				argumentText, "", eventId, "schedule", opcode1, opcode2);
            break;
    }
	if (commandCompleted)
	{
		string completionResult = responseCodeName(result.responesCode);
		string intermediateCode;
		string intermediateName;
		if (result.responesCode == ResponseCode::OK
				&& result.hasIntermediateResponseCode)
		{
			intermediateCode =
				to_string(static_cast<int>(result.intermediateResponseCode));
			intermediateName =
				intermediateResponseCodeName(result.intermediateResponseCode);
			if (result.intermediateResponseCode != 0x00)
				completionResult = intermediateName;
		}
		logCtaEvent(
			"command_completed", "outbound", commandName,
			completionResult, argumentText,
			"", eventId, "schedule", opcode1, opcode2,
			"", "", "", "", intermediateCode, intermediateName);

		const bool shouldScheduleReadback = tolower(cmd) == 'a'
				&& result.responesCode == ResponseCode::OK
				&& result.hasIntermediateResponseCode
				&& result.intermediateResponseCode == 0x00;
		return shouldScheduleReadback;
	}
	return false;
}


void commodity_service_loop(
	std::shared_ptr<ICEA2045DeviceUCM> dev,
	bool scheduleEnabled){
    fstream file;
    time_t now;
    string header,line,lines;
    const chrono::seconds schedulerInterval(1);
    const chrono::seconds commodityInterval(60);
    const chrono::seconds operationalStateInterval(30);
    chrono::steady_clock::time_point nextCommodityRead = chrono::steady_clock::now();
    chrono::steady_clock::time_point nextOperationalStateRead = chrono::steady_clock::now();
    while (1)
    {
	if (scheduleEnabled)
	{
	bool scheduleChanged = false;
	file.clear();
	file.open("schedule.csv", ofstream::in);
	if (!file.is_open())
		cout<<"FAILED TO OPEN SCHEDULE.CSV"<<endl;
	// prime the buffer -- skip the header
	getline(file,header);
    lines = "# time,command,argument,event_id,value,units,efficiency\n";
	while (getline(file,line))
	{
	    if (line.empty() || line[0] == '#')
	        continue;

	    string timestampText, commandText, argumentText, eventId, valueText, unitsText, efficiencyText;
	    stringstream row(line);
	    getline(row, timestampText, ',');
	    getline(row, commandText, ',');
	    getline(row, argumentText, ',');
	    getline(row, eventId, ',');
	    getline(row, valueText, ',');
	    getline(row, unitsText, ',');
	    getline(row, efficiencyText, ',');
	    const auto trim = [](string& value)
	    {
	        while (!value.empty() && isspace(static_cast<unsigned char>(value.front())))
	            value.erase(value.begin());
	        while (!value.empty() && isspace(static_cast<unsigned char>(value.back())))
	            value.pop_back();
	    };
	    trim(timestampText);
	    trim(commandText);
	    trim(argumentText);
	    trim(eventId);
	    trim(valueText);
	    trim(unitsText);
	    trim(efficiencyText);

	    time_t t;
	    unsigned long argumentValue = 0;
	    unsigned long advancedValue = 0;
	    unsigned long advancedUnits = 0;
	    unsigned long advancedEfficiency = 0;
	    try
	    {
	        size_t timestampEnd = 0;
	        const long long timestampValue = stoll(timestampText, &timestampEnd);
	        if (timestampEnd != timestampText.size())
	            throw invalid_argument("timestamp contains unexpected characters");
	        t = static_cast<time_t>(timestampValue);
	        if (!argumentText.empty())
	        {
	            size_t argumentEnd = 0;
	            argumentValue = stoul(argumentText, &argumentEnd);
	            if (argumentEnd != argumentText.size())
	                throw invalid_argument("argument contains unexpected characters");
	        }
	        if (!valueText.empty())
	        {
	            size_t valueEnd = 0;
	            advancedValue = stoul(valueText, &valueEnd);
	            if (valueEnd != valueText.size())
	                throw invalid_argument("advanced value contains unexpected characters");
	        }
	        if (!unitsText.empty())
	        {
	            size_t unitsEnd = 0;
	            advancedUnits = stoul(unitsText, &unitsEnd);
	            if (unitsEnd != unitsText.size())
	                throw invalid_argument("advanced units contain unexpected characters");
	        }
	        if (!efficiencyText.empty())
	        {
	            size_t efficiencyEnd = 0;
	            advancedEfficiency = stoul(efficiencyText, &efficiencyEnd);
	            if (efficiencyEnd != efficiencyText.size())
	                throw invalid_argument("advanced efficiency contains unexpected characters");
	        }
	    }
	    catch (const exception& error)
	    {
	        LOG(ERROR) << "invalid schedule row retained: " << line
	                   << " (" << error.what() << ")";
	        lines += line + "\n";
	        continue;
	    }

	    if (commandText.size() != 1 || string("aselgcov").find(commandText[0]) == string::npos)
	    {
	        LOG(ERROR) << "invalid schedule command retained: " << line;
	        lines += line + "\n";
	        continue;
	    }
	    char cmd = commandText[0];
	    if (tolower(cmd) == 'a')
	    {
	        if (argumentText.empty() || valueText.empty() || unitsText.empty()
	            || argumentValue > 0xFFFF
	            || advancedValue == 0 || advancedValue > 0xFFFE
	            || advancedUnits > 0x03
	            || (!efficiencyText.empty() && advancedEfficiency > 10))
	        {
	            LOG(ERROR) << "invalid advanced load-up arguments retained: " << line;
	            lines += line + "\n";
	            continue;
	        }
	    }
	    else if (tolower(cmd) != 'v'
	             && (argumentValue > 0xFF || !valueText.empty() || !unitsText.empty()
	             || !efficiencyText.empty())
	    )
	    {
	        LOG(ERROR) << "invalid Basic DR arguments retained: " << line;
	        lines += line + "\n";
	        continue;
	    }

		// grab time
		time(&now);
		if (now >= t)
		{
		    // passed & should act on it
		    scheduleChanged = true;
		    cout<<t<<','<<cmd<<','<<argumentValue<<" (PASSED!)\n";
		    try
		    {
		    const bool shouldScheduleReadback = perform_command(
		        cmd,
		        static_cast<unsigned int>(argumentValue),
		        static_cast<unsigned int>(advancedValue),
		        static_cast<unsigned int>(advancedUnits),
		        !efficiencyText.empty(),
		        static_cast<unsigned int>(advancedEfficiency),
		        eventId,
		        dev);
		    if (shouldScheduleReadback)
		    {
		        const unsigned int readbackDelaySeconds = 10;
		        time_t readbackTime;
		        time(&readbackTime);
		        lines += to_string(readbackTime + readbackDelaySeconds)
		            + ",v," + to_string(readbackDelaySeconds) + ","
		            + eventId + ",,,\n";
		    }
		    }
		    catch (const exception& error)
		    {
		        logCtaEvent(
		            "command_exception",
		            "outbound",
		            scheduledCommandName(cmd),
		            "error",
		            argumentText,
		            error.what(),
		            eventId,
		            "schedule",
		            scheduledCommandOpcode1(cmd),
		            tolower(cmd) == 'a' ? "" : argumentText);
		    }
		}
		else
		{
		    // did not pass, leave it be for the future
		    lines += to_string(t) + ',' + cmd + ',' + argumentText + ','
		        + eventId + ',' + valueText + ',' + unitsText + ','
		        + efficiencyText + "\n";
		}
	}
	file.close();
	if (scheduleChanged)
	{
	    file.clear();
	    file.open("schedule.csv",ofstream::out);
	    if (!file.is_open())
	    {
	        cout<<"FAILED TO OPEN SCHEDULE.CSV FOR UPDATE"<<endl;
	    }
	    else
	    {
	        // Remove commands that were dispatched while retaining future rows.
	        file << lines;
	        file.close();
		}
	}
	}
	// ------------------------------ end of scheduler ----------------------
	// Commodity polling uses a 60-second clock while operational-state polling
	// uses a 30-second clock.
	if (chrono::steady_clock::now() >= nextCommodityRead)
	{
	    // dev->intermediateGetDeviceInformation().get();
	    logCtaEvent(
	        "query_sent", "outbound", "get_commodity", "pending",
	        "", "", "", "periodic", "6", "0");
	    try
	    {
	        ResponseCodes commodityResult = dev->intermediateGetCommodity().get();
			string completionResult = responseCodeName(commodityResult.responesCode);
			string intermediateCode;
			string intermediateName;
			if (commodityResult.responesCode == ResponseCode::OK
					&& commodityResult.hasIntermediateResponseCode)
			{
				intermediateCode = to_string(
					static_cast<int>(commodityResult.intermediateResponseCode));
				intermediateName = intermediateResponseCodeName(
					commodityResult.intermediateResponseCode);
				if (commodityResult.intermediateResponseCode != 0x00)
					completionResult = intermediateName;
			}
	        logCtaEvent(
	            "query_completed", "outbound", "get_commodity",
	            completionResult,
	            "", "", "", "periodic", "6", "0",
				"", "", "", "", intermediateCode, intermediateName);
	    }
	    catch (const exception& error)
	    {
	        logCtaEvent(
	            "query_exception", "outbound", "get_commodity", "error",
	            "", error.what(), "", "periodic", "6", "0");
	    }
	    nextCommodityRead += commodityInterval;
	    if (nextCommodityRead <= chrono::steady_clock::now())
	        nextCommodityRead = chrono::steady_clock::now() + commodityInterval;
	}

	if (chrono::steady_clock::now() >= nextOperationalStateRead)
	{
	    logCtaEvent(
	        "query_sent", "outbound", "query_operational_state", "pending",
	        "", "", "", "periodic", "18", "0");
	    try
	    {
	        ResponseCodes stateResult = dev->basicQueryOperationalState().get();
	        logCtaEvent(
	            "query_completed", "outbound", "query_operational_state",
	            responseCodeName(stateResult.responesCode),
	            "", "", "", "periodic", "18", "0");
	    }
	    catch (const exception& error)
	    {
	        logCtaEvent(
	            "query_exception", "outbound", "query_operational_state", "error",
	            "", error.what(), "", "periodic", "18", "0");
	    }

	    nextOperationalStateRead += operationalStateInterval;
	    if (nextOperationalStateRead <= chrono::steady_clock::now())
	        nextOperationalStateRead = (
	            chrono::steady_clock::now() + operationalStateInterval);
	}
        this_thread::sleep_for(schedulerInterval);
    }
}
