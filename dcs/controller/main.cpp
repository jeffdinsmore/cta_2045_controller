/*
 * ref: https://github.com/epri-dev/CTA-2045-UCM-CPP-Library.git
 * author: Midrar Adham
 */

#include "UCMImpl.h"

#include <easylogging++.h>

#include <cea2045/device/DeviceFactory.h>

#include <cea2045/communicationport/CEA2045SerialPort.h>

#include <unistd.h>

#include <thread>

//#include <QCoreApplication>
#include <iostream>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>

using namespace cea2045;

INITIALIZE_EASYLOGGINGPP

#include <cea2045/util/MSTimer.h>

void perform_command(char cmd, shared_ptr<ICEA2045DeviceUCM> dev){
    switch (tolower(cmd)){
        case 's':
            cout<<"shedding"<<endl;
	    dev->basicShed(0);
            break;
        case 'e':
	    dev->basicEndShed(0);
            cout<<"endshedding"<<endl;
            break;
        case 'l':
            cout<<"loading up"<<endl;
	    dev->basicLoadUp(0);
            break;
        case 'g':
            cout<<"grid emergency"<<endl;
	    dev->basicGridEmergency(0);
            break;
        case 'c':
            cout<<"critical peak event"<<endl;
	    dev->basicCriticalPeakEvent(0);
            break;

        default:
            break;
    }
    return;
}


void commodity_service_loop(shared_ptr<ICEA2045DeviceUCM> dev){
    fstream file;
    int t;
    time_t now;
    char cmd,comma;
    string header,lines;
    while (1)
    {
	file.open("schedule.csv", ofstream::in);
	if (!file.is_open())
		cout<<"FAILED TO OPEN SCHEDULE.CSV"<<endl;
	// prime the buffer -- skip the header
	getline(file,header);
	lines = header+ "\n";
	while (file>>t>>comma>>cmd)
	{
		// grab time
		time(&now);
		if (now >= t)
		{
		    // passed & should act on it
		    cout<<t<<comma<<cmd<<" (PASSED!)\n";
		    perform_command(cmd,dev);
		}
		else
		{
		    // did not pass, leave it be for the future
		    cout<<t<<cmd<<" (STILL!)\n";
		    lines+= to_string(t) + comma + cmd + "\n";
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
	dev->intermediateGetCommodity().get();
        dev->basicQueryOperationalState().get();
        sleep(60);
    }
}
int main()
{
	MSTimer timer;
	bool shutdown = false;

	CEA2045SerialPort sp("/dev/ttyUSB0");
	UCMImpl ucm;
	ResponseCodes responseCodes;

	if (!sp.open())
	{
		LOG(ERROR) << "failed to open serial port: " << strerror(errno);
		return 0;
	}

	//shared_ptr<ICEA2045DeviceUCM> device = make_shared<DeviceFactory::createUCM(&sp, &ucm)>();
    //auto device = mak
	//shared_ptr<ICEA2045DeviceUCM> device = make_shared<DeviceFactory::createUCM>(&sp,&ucm);
    shared_ptr<ICEA2045DeviceUCM> device(DeviceFactory::createUCM(&sp,&ucm));
    //device = make_shared<ICEA2045DeviceUCM>();
    //device = DeviceFactory::createUCM(&sp,&ucm);

	device->start();


	LOG(INFO) << "starting commodity service...";
    thread commodity(commodity_service_loop,device);
    commodity.detach();
    sleep(5);
	while (!shutdown)
	{
        cout<<"c- CriticalPeakEvent\n";
        cout<<"e- Endshed\n";
        cout<<"g- GridEmergency\n";
        cout<<"l- Loadup\n";
        cout<<"o- OutsideCommunication\n";
        cout<<"s- Shed\n";
        cout<<"q- Quit\n";
        cout<<"enter choice: ";
		char c = getchar();

		switch (c)
		{
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

			case 's':
    				cout << "Sending SHED..." << endl;
    				device->basicShed(0).get();
					cout << "Loading..."<< endl;
    				sleep(15);

    				cout << "Querying operational state after SHED..." << endl;
    				device->basicQueryOperationalState().get();
    				break;
			
			case '\n':
				break;

			case 'o':
				device->basicOutsideCommConnectionStatus(OutsideCommuncatonStatusCode::Found);
				break;


			case 'q':
				shutdown = true;
				break;

			default:
				LOG(WARNING) << "invalid command";
				break;
		}
	}

	device->shutDown();

	//delete (device);

	return 0;


}
