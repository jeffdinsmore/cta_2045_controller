/*
 * ref: https://github.com/epri-dev/CTA-2045-UCM-CPP-Library.git
 * author: Midrar Adham
 */

#include "UCMImpl.h"
#include "CtaEventLog.h"

#include <easylogging++.h>

#include <cea2045/device/DeviceFactory.h>

#include <cea2045/communicationport/CEA2045SerialPort.h>

#include <unistd.h>

#include <thread>

//#include <QCoreApplication>
#include <iostream>
#include <cctype>
#include <sstream>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>

using namespace cea2045;

INITIALIZE_EASYLOGGINGPP

#include <cea2045/util/MSTimer.h>

void perform_command(char cmd, unsigned int argument, unsigned int value, unsigned int units, const string& eventId, std::shared_ptr<ICEA2045DeviceUCM> dev);
void commodity_service_loop(std::shared_ptr<ICEA2045DeviceUCM> dev);

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
	case 'o': return "outside_communication";
	default: return "unknown";
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

int main()
{
	MSTimer timer;
	bool shutdown = false;

	CEA2045SerialPort sp("/dev/ttyUSB0");
	UCMImpl ucm;
	logCtaEvent("controller_started", "internal", "controller", "started");

	if (!sp.open())
	{
		LOG(ERROR) << "failed to open serial port: " << strerror(errno);
		logCtaEvent("serial_open", "internal", "serial_port", "error", "/dev/ttyUSB0", strerror(errno));
		return 0;
	}
	logCtaEvent("serial_open", "internal", "serial_port", "ok", "/dev/ttyUSB0");

	//shared_ptr<ICEA2045DeviceUCM> device = make_shared<DeviceFactory::createUCM(&sp, &ucm)>();
    //auto device = mak
	//shared_ptr<ICEA2045DeviceUCM> device = make_shared<DeviceFactory::createUCM>(&sp,&ucm);
    std::shared_ptr<ICEA2045DeviceUCM> device(DeviceFactory::createUCM(&sp,&ucm));
    //device = make_shared<ICEA2045DeviceUCM>();
    //device = DeviceFactory::createUCM(&sp,&ucm);

	device->start();
	logCtaEvent("communication_started", "internal", "cta2045", "started");


	LOG(INFO) << "starting commodity service...";
    std::thread commodity(commodity_service_loop,device);
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
		cout<<"v- Enable advanced load up capability\n";
		cout<<"x- Disable advanced load up capability\n";
		cout<<"z- Quit and return operation to normal\n";
        cout<<"q- Quit\n";
        cout<<"enter choice: ";
		char c = getchar();

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
				logCtaEvent("command_sent", "outbound", "run_normal", "pending", "0", "source=shutdown");
				{
					ResponseCodes result = device->basicEndShed(0).get();
					logCtaEvent("command_completed", "outbound", "run_normal", responseCodeName(result.responesCode), "0", "source=shutdown");
				}
				cout << "Loading..."<< endl;
				sleep(2);

				cout << "Querying operational state after normal command..." << endl;
				device->basicQueryOperationalState().get();
				shutdown = true;
				break;
			case '\n':
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
	logCtaEvent("controller_stopped", "internal", "controller", "stopped");

	//delete (device);

	return 0;


}


void perform_command(char cmd, unsigned int argument, unsigned int value, unsigned int units, const string& eventId, std::shared_ptr<ICEA2045DeviceUCM> dev){
	const string commandName = scheduledCommandName(cmd);
	string argumentText = to_string(argument);
	if (tolower(cmd) == 'a')
		argumentText = "duration_minutes=" + to_string(argument)
			+ ";value=" + to_string(value)
			+ ";units=" + to_string(units);
	logCtaEvent("command_sent", "outbound", commandName, "pending", argumentText, "source=schedule", eventId);
	ResponseCodes result;
	bool commandCompleted = true;
    switch (tolower(cmd)){
		case 'a':
			cout << "advanced load up"<< endl;
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
			logCtaEvent("command_rejected", "internal", commandName, "invalid_command", argumentText, "source=schedule", eventId);
            break;
    }
	if (commandCompleted)
		logCtaEvent("command_completed", "outbound", commandName, responseCodeName(result.responesCode), argumentText, "source=schedule", eventId);
    return;
}


void commodity_service_loop(std::shared_ptr<ICEA2045DeviceUCM> dev){
    fstream file;
    time_t now;
    string header,line,lines;
    while (1)
    {
	file.open("schedule.csv", ofstream::in);
	if (!file.is_open())
		cout<<"FAILED TO OPEN SCHEDULE.CSV"<<endl;
	// prime the buffer -- skip the header
	getline(file,header);
	lines = "# time,command,argument,event_id,value,units\n";
	while (getline(file,line))
	{
	    if (line.empty() || line[0] == '#')
	        continue;

	    string timestampText, commandText, argumentText, eventId, valueText, unitsText;
	    stringstream row(line);
	    getline(row, timestampText, ',');
	    getline(row, commandText, ',');
	    getline(row, argumentText, ',');
	    getline(row, eventId, ',');
	    getline(row, valueText, ',');
	    getline(row, unitsText, ',');
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

	    time_t t;
	    unsigned long argumentValue = 0;
	    unsigned long advancedValue = 0;
	    unsigned long advancedUnits = 0;
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
	    }
	    catch (const exception& error)
	    {
	        LOG(ERROR) << "invalid schedule row retained: " << line
	                   << " (" << error.what() << ")";
	        lines += line + "\n";
	        continue;
	    }

	    if (commandText.size() != 1 || string("aselgco").find(commandText[0]) == string::npos)
	    {
	        LOG(ERROR) << "invalid schedule command retained: " << line;
	        lines += line + "\n";
	        continue;
	    }
	    char cmd = commandText[0];
	    if (tolower(cmd) == 'a')
	    {
	        if (argumentText.empty() || valueText.empty() || unitsText.empty()
	            || argumentValue == 0 || argumentValue > 0xFFFF
	            || advancedValue == 0 || advancedValue > 0xFFFE
	            || advancedUnits > 0x03)
	        {
	            LOG(ERROR) << "invalid advanced load-up arguments retained: " << line;
	            lines += line + "\n";
	            continue;
	        }
	    }
	    else if (argumentValue > 0xFF || !valueText.empty() || !unitsText.empty())
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
		    cout<<t<<','<<cmd<<','<<argumentValue<<" (PASSED!)\n";
		    try
		    {
		    perform_command(
		        cmd,
		        static_cast<unsigned int>(argumentValue),
		        static_cast<unsigned int>(advancedValue),
		        static_cast<unsigned int>(advancedUnits),
		        eventId,
		        dev);
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
		            eventId);
		    }
		}
		else
		{
		    // did not pass, leave it be for the future
		    cout<<t<<cmd<<" (STILL!)\n";
		    lines += to_string(t) + ',' + cmd;
		    if (!argumentText.empty())
		        lines += ',' + argumentText;
		    else if (!eventId.empty())
		        lines += ',';
		    if (!eventId.empty())
		        lines += ',' + eventId;
		    else if (!valueText.empty() || !unitsText.empty())
		        lines += ',';
		    if (!valueText.empty())
		        lines += ',' + valueText;
		    else if (!unitsText.empty())
		        lines += ',';
		    if (!unitsText.empty())
		        lines += ',' + unitsText;
		    lines += "\n";
		}
	}
	file.close();
	file.open("schedule.csv",ofstream::out);
	if (!file.is_open())
		cout<<"FAILED TO OPEN SCHEDULE.CSV"<<endl;
	// rewrite the commands
	file << lines<<endl;
	file.close();
	// ------------------------------ end of scheduler ----------------------
	// send routine commands (commodity read & op status)
	// dev->intermediateGetDeviceInformation().get();
	logCtaEvent("query_sent", "outbound", "get_commodity", "pending", "", "source=periodic");
	try
	{
	    ResponseCodes commodityResult = dev->intermediateGetCommodity().get();
	    logCtaEvent("query_completed", "outbound", "get_commodity", responseCodeName(commodityResult.responesCode), "", "source=periodic");
	}
	catch (const exception& error)
	{
	    logCtaEvent("query_exception", "outbound", "get_commodity", "error", "", error.what());
	}

	logCtaEvent("query_sent", "outbound", "query_operational_state", "pending", "", "source=periodic");
	try
	{
	    ResponseCodes stateResult = dev->basicQueryOperationalState().get();
	    logCtaEvent("query_completed", "outbound", "query_operational_state", responseCodeName(stateResult.responesCode), "", "source=periodic");
	}
	catch (const exception& error)
	{
	    logCtaEvent("query_exception", "outbound", "query_operational_state", "error", "", error.what());
	}
        sleep(60);
    }
}
