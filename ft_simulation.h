/*-------------------------------------------------------
FAST-TrIPs: Flexible Assignment and Simulation Tool for Transit and Intermodal Passengers
Copyright (C) 2013 by Alireza Khani
Released under the GNU General Public License, version 2.
-------------------------------------------------------
Code primarily written by Alireza Khani
Under supervision of Mark Hickman

Contact:
    Alireza Khani:  akhani@utexas.edu or akhani@email.arizona.edu
    Mark Hickman:   m.hickman1@uq.edu.au
-------------------------------------------------------
This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program. If not, see <http://www.gnu.org/licenses/>.
-------------------------------------------------------*/

using namespace std;

int		initializeSimulatoin(){
	int									tmpInt, noOfPassengers, tmpPassengerStatus;
	string								fileStr, tmpPath;
	list<stop*>::iterator				tmpStopListIter;
	list<trip*>::iterator				tmpTripListIter;
	list<passenger*>::iterator			tmpPassengerListIter;
	passenger*							tmpPassengerPntr;

	passengers2transfer.clear();
	for(tmpStopListIter=stopList.begin();tmpStopListIter!=stopList.end();tmpStopListIter++){
		(*tmpStopListIter)->resetStopForSimulation();
	}
	for(tmpTripListIter=tripList.begin();tmpTripListIter!=tripList.end();tmpTripListIter++){
		(*tmpTripListIter)->resetTripForSimulation();
	}
	noOfPassengers = 0;
	for(tmpPassengerListIter=passengerList.begin();tmpPassengerListIter!=passengerList.end();tmpPassengerListIter++){
		tmpPassengerPntr = (*tmpPassengerListIter);
		tmpPassengerStatus = tmpPassengerPntr->getPassengerStatus();
		tmpPath = tmpPassengerPntr->getAssignedPath();
        if(tmpPath!=""){
            tmpPassengerPntr->resetPathInfo();
			tmpInt = tmpPassengerPntr->initializePath();
			tmpPassengerPntr->setPassengerStatus(0);
			noOfPassengers++;
		}
	}

	//availableCapacity.clear();            //May result in passengers flip-floping if clear the available capacities!
                                            //Will converge otherwise, but some shorter paths may become available in larger iterations although bot being used, this is similar to Hyunsoo's example!
    return noOfPassengers;
}
//////////////////////////////////////////////////////////////////////////////////////////////
int		createPassengers(int _t1, int _t2){
	string								buf, tmpPassengerId, tmpVehicleId;
	int									tmpPassengerStatus, tmpStartTime, noOfPassengerCreated;
	map<string,passenger*>::iterator	tmpPassengerIter;
	passenger*							passengerPntr;

	noOfPassengerCreated = 0;
	for(tmpPassengerIter=passengerSet.begin();tmpPassengerIter!=passengerSet.end();tmpPassengerIter++){
		passengerPntr = (*tmpPassengerIter).second;
        tmpPassengerStatus = passengerPntr->getPassengerStatus();
		tmpStartTime = 60 * passengerPntr->getStartTime();
		if (tmpPassengerStatus==0 && tmpStartTime >= _t1 && tmpStartTime < _t2){
			passengers2transfer.push_back(passengerPntr);
			passengerPntr->setPassengerStatus(1);	//walking
			noOfPassengerCreated++;
		}
	}
	return noOfPassengerCreated;
}
//////////////////////////////////////////////////////////////////////////////////////////////
int		transferPassengers(int _t){
	string							tmpIn, tmpPassengerId, tmpBoardingStop, tmpNextTrip, tmpExperiencedPath;
	int								tmpPassCntr, tmpNoOfPassengetrs, tmpAlightingTime, tmpWalkingTime,
									tmpMissingCase, tmpNumPassengersArrived, tmpNumPassengersRemoved;
	list<passenger*>::iterator		tmpPassenger2transferIter;
	passenger*						passengerPntr;

	tmpNoOfPassengetrs = passengers2transfer.size();
	tmpPassenger2transferIter = passengers2transfer.begin();
	tmpNumPassengersArrived = 0;
	tmpNumPassengersRemoved = 0;
	tmpMissingCase = 0;

	for(tmpPassCntr=1;tmpPassCntr<=tmpNoOfPassengetrs;tmpPassCntr++){
		passengerPntr = NULL;
		passengerPntr = *tmpPassenger2transferIter;
		tmpPassengerId = passengerPntr->getPassengerId();

		tmpBoardingStop = passengerPntr->getCurrentBoardingStopId();
		tmpAlightingTime = 60 * passengerPntr->getAlightingTime();
		tmpWalkingTime = 60 * passengerPntr->getCurrentWalkingTime();

		if(_t >= tmpAlightingTime + tmpWalkingTime){
			if(tmpBoardingStop=="-101"){
				passengers2transfer.remove(passengerPntr);
				passengerPntr->setPassengerStatus(5);	//arrived
				passengerPntr->setEndTime((tmpAlightingTime + tmpWalkingTime)/60.0);
				//cout <<"\ttime: "<<_t/60.0<<" Passenger "<<tmpPassengerId<<" arrieved at destination."<<endl;
				tmpNumPassengersRemoved++;
				tmpNumPassengersArrived++;
				tmpExperiencedPath = passengerPntr->getExperiencedPath();
			}else{
				stopSet[tmpBoardingStop]->addPassenger(tmpPassengerId);
				passengers2transfer.remove(passengerPntr);
				passengerPntr->addArrivalTime((tmpAlightingTime + tmpWalkingTime)/60.0);
				passengerPntr->setPassengerStatus(2);	//waiting
				//cout <<"\ttime: "<<_t/60.0<<" Passenger "<<tmpPassengerId<<" was transfered to stop "<<tmpBoardingStop<<endl;

				tmpNumPassengersRemoved++;
			}
			tmpPassenger2transferIter = passengers2transfer.begin();
			advance(tmpPassenger2transferIter,tmpPassCntr-tmpNumPassengersRemoved);
		}else{
			tmpPassenger2transferIter++;
		}
	}
	return	tmpNumPassengersArrived;
}
//////////////////////////////////////////////////////////////////////////////////////////////
int		alightPassengers(int _t, string _tripId, string _stopId){

	string			fileStr, tmpPassengerId, tmpTripId;
	int				tmpPassCntr, tmpNoOfOnBoardPassengers, tmpNoOfAlightings;
	stop*			tmpStopPntr;
	trip*			tmpTripPntr;
	passenger*		tmpPassengerPntr;

	tmpTripPntr = tripSet[_tripId];
	tmpStopPntr = stopSet[_stopId];
	tmpNoOfAlightings = 0;

	tmpNoOfOnBoardPassengers = tmpTripPntr->getNoOfOnBoardPassengers();
	if(tmpNoOfOnBoardPassengers > 0){
		for(tmpPassCntr=1;tmpPassCntr<=tmpNoOfOnBoardPassengers;tmpPassCntr++){
			tmpPassengerId = tmpTripPntr->getPassengerId(tmpPassCntr-1);
			tmpPassengerPntr = passengerSet[tmpPassengerId];
			if(tmpPassengerPntr->getCurrentAlightingStopId()==_stopId){
				//cout<<"\ttime: "<<_t/60.0<<" passenger "<<tmpPassengerId<<" got off at stop "<<_stopId<<"."<<endl;
				tmpPassengerPntr->setPassengerStatus(1);	//walking
				tmpPassengerPntr->increasePathIndex();
				tmpPassengerPntr->addAlightingTime(_t/60.0);
				passengers2transfer.push_back(tmpPassengerPntr);
				tmpNoOfAlightings = tmpNoOfAlightings + 1;
				tmpTripPntr->removePassenger(tmpPassCntr-1);
				tmpNoOfOnBoardPassengers = tmpTripPntr->getNoOfOnBoardPassengers();
				tmpPassCntr = tmpPassCntr - 1;
			}
		}
	}
	tmpTripPntr->setAlightings(tmpNoOfAlightings);
	return tmpNoOfAlightings;
}
//////////////////////////////////////////////////////////////////////////////////////////////
int		boardPassengers(int _t, string _tripId, string _stopId){

	string			tmpPassengerId, tmpTripId;
	int				tmpPassCntr, tmpFreeCapacity, tmpNoOfOnBoardPassengers, tmpNoOfWaitingPassengers,
					tmpNoOfBoardings, tmpNoOfMissings, tmpMissingCase;

	//For Available Capacity Definition
	string			fromToAt;
    float			tmpArrivaltime, tmpLatestArrivalTime;

	stop*			tmpStopPntr;
	trip*			tmpTripPntr;
	passenger*		tmpPassengerPntr;

	tmpTripPntr = tripSet[_tripId];
	tmpStopPntr = stopSet[_stopId];
	tmpNoOfBoardings = 0;
	tmpNoOfMissings = 0;

	tmpNoOfWaitingPassengers = tmpStopPntr->getNoOfWaitingPassenegrs();
	if(tmpNoOfWaitingPassengers > 0){

		tmpTripId = tmpTripPntr->getTripId();
		tmpFreeCapacity = tmpTripPntr->getFreeCapacity();
		for(tmpPassCntr=1;tmpPassCntr<=tmpNoOfWaitingPassengers;tmpPassCntr++){
			tmpPassengerId = tmpStopPntr->getPassengerId(tmpPassCntr-1);
			tmpPassengerPntr = passengerSet[tmpPassengerId];
			if(tmpPassengerPntr->getCurrentTripId() == _tripId){
				if(tmpFreeCapacity==0){
					tmpMissingCase = 4;		//No space on the bus for the passenger to board!
					tmpPassengerPntr->setPassengerStatus(4);	//missed
					//cout <<"\tPassenger "<<tmpPassengerId<<" missed bus #"<<_tripId<<" at stop "<<_stopId<<" at time "<<_t/60.0<<"\tMC: "<<tmpMissingCase<<endl;
					tmpNoOfMissings = tmpNoOfMissings + 1;

					fromToAt = tmpTripId + "," + _stopId;
					tmpArrivaltime = tmpPassengerPntr->getArrivalTime();
					if(availableCapacity.find(fromToAt)==availableCapacity.end()){
						availableCapacity[fromToAt] = tmpArrivaltime;
					}else{
						tmpLatestArrivalTime = availableCapacity[fromToAt];
						if(tmpArrivaltime < tmpLatestArrivalTime){
							availableCapacity[fromToAt] = tmpArrivaltime;
						}
					}
				}else{
					tmpTripPntr->addPassenger(tmpPassengerId);
					tmpPassengerPntr->addBoardingTime(_t/60.0);
					tmpPassengerPntr->setPassengerStatus(3);	//on board
					//cout <<"\ttime: "<<_t/60.0<<"  Passenger "<<tmpPassengerId<<" got on the vehicle #"<<_tripId<<"."<<endl;
					tmpNoOfBoardings = tmpNoOfBoardings + 1;
					tmpFreeCapacity = tmpFreeCapacity - 1;
				}
				tmpStopPntr->removePassenger(tmpPassCntr-1);
				tmpNoOfWaitingPassengers = tmpNoOfWaitingPassengers - 1;
				tmpPassCntr = tmpPassCntr - 1;
			}
		}
	}
	tmpTripPntr->setResidualCapacity(-1);
	tmpTripPntr->setBoardings(tmpNoOfBoardings);
	tmpNoOfOnBoardPassengers = tmpTripPntr->getNoOfOnBoardPassengers();
	tmpTripPntr->setOnBoards(tmpNoOfOnBoardPassengers);
	return	tmpNoOfMissings;
}
//////////////////////////////////////////////////////////////////////////////////////////////
int		calculateDwellTime(int _t, string _tripId, string _stopId){

	string			tmpTripId;
	int				tmpNoOfBoardings, tmpNoOfAlightings, tmpDwellTime, tmpRouteType;
	stop*			tmpStopPntr;
	trip*			tmpTripPntr;

	tmpTripPntr = tripSet[_tripId];
	tmpStopPntr = stopSet[_stopId];
	tmpNoOfBoardings = tmpTripPntr->getNoOfBoardings();
	tmpNoOfAlightings = tmpTripPntr->getNoOfAlightings();

	tmpRouteType = tmpTripPntr->getRouteType();
	if(tmpRouteType==0){
		tmpDwellTime = 30;
	}else{
		if(tmpNoOfBoardings>0 || tmpNoOfAlightings>0){
			tmpDwellTime = 4 + max(4*tmpNoOfBoardings,2*tmpNoOfAlightings);
		}else{
			tmpDwellTime = 0;
		}
	}
	if(tmpDwellTime<0){
		cout <<"Error - Negative Dwell Time!"<<endl;
	}
	tmpTripPntr->setDwellTime(tmpDwellTime);
	tmpTripPntr->increaseStopIndex();
	return	tmpDwellTime;
}
//////////////////////////////////////////////////////////////////////////////////////////////
long long		simulation(){
	int					t, tmpOut, tmpEventTime, tmpLastEventTime, numEventsSimulated, minTime, maxTime,
						numAssignedPassengers, numDepartedPassengers, numArrivedPassengers, numCompletedPassengers, numMissingPassengers;
	string				buf, tmpEventStr, tmpEventTrip, tmpEventStop, tmpEventType;
    char                chr[99];
	vector<string>		tokens;
	trip*				tripPntr;

	numAssignedPassengers = initializeSimulatoin();

	numEventsSimulated = 0;
	tmpLastEventTime = 0;
	numDepartedPassengers = 0;
	numArrivedPassengers = 0;
	numCompletedPassengers = 0;
    numMissingPassengers = 0;
	tmpEventStr = eventList.front();
	stringstream ss1(tmpEventStr);
	buf.clear();
	tokens.clear();
	if (getline(ss1, buf, ',')){
		tokens.push_back(buf);
	}
	minTime = 3600*((atoi(tokens[0].c_str())-1800)/3600);
	tmpEventStr = eventList.back();
	stringstream ss2(tmpEventStr);
	buf.clear();
	tokens.clear();
	if (getline(ss2, buf, ',')){
		tokens.push_back(buf);
	}
	maxTime = 3600*((atoi(tokens[0].c_str())+1800)/3600) + 3600;
	for(t=minTime;t<=maxTime;t=t+1){
		if(t%3600==0){
			numCompletedPassengers = numCompletedPassengers + numArrivedPassengers;
			//if(numDepartedPassengers>0)	cout <<"\t\t"<<numDepartedPassengers<<"\tpassengers departed the origin."<<endl;
			//if(numArrivedPassengers>0)	cout <<"\t\t"<<numArrivedPassengers<<"\tpassengers arrived to the destination."<<endl;
			cout <<" t = "<<t/3600<<":00\ttime elapsed: "<<"N/A"<<endl;
			numDepartedPassengers = 0;
			numArrivedPassengers = 0;
			tmpOut = createPassengers(t, t+3600);
			numDepartedPassengers = numDepartedPassengers + tmpOut;
		}
		tmpOut = transferPassengers(t);
		numArrivedPassengers = numArrivedPassengers + tmpOut;
		while(eventList.size() > 0){
			tmpEventStr = eventList.front();
			stringstream ss(tmpEventStr);
			buf.clear();
			tokens.clear();
			while (getline(ss, buf, ',')){
				tokens.push_back(buf);
			}
			tmpEventTime = atoi(tokens[0].c_str());
			if(tmpEventTime == t){
				eventList.pop_front();
				eventList.push_back(tmpEventStr);
				tmpLastEventTime = tmpEventTime;
				tmpEventType = tokens[1];	//FNS
				tmpEventTrip = tokens[2];
				tmpEventStop = tokens[3];
				tripPntr = tripSet[tmpEventTrip];

				if(tmpEventType=="a"){
					alightPassengers(tmpEventTime, tmpEventTrip, tmpEventStop);
				}else if(tmpEventType=="d"){
					tmpOut = boardPassengers(tmpEventTime, tmpEventTrip, tmpEventStop);
                    numMissingPassengers = numMissingPassengers + tmpOut;
					calculateDwellTime(tmpEventTime, tmpEventTrip, tmpEventStop);
				}else{
					cout <<"Error - Incorrect Event Type!"<<endl;
				}

				if(passengers2transfer.size()>0){
					tmpOut = transferPassengers(t);
					numArrivedPassengers = numArrivedPassengers + tmpOut;
				}
				numEventsSimulated++;
			}else if(tmpEventTime < tmpLastEventTime){
				numCompletedPassengers = numCompletedPassengers + numArrivedPassengers;
                return	numCompletedPassengers;
			}else{
				if((t+1)%3600!=1){
					t = max(t, min(tmpEventTime-1,3600*(t/3600)+3599));
				}
				break;
			}
		}
	}
	return	numCompletedPassengers;
}
//////////////////////////////////////////////////////////////////////////////////////////////
int		printLoadProfile(){
	int									i, tmpMaxSeq, tmpNumStopTimes, tmpDwellTime, tmpVehLoad, tmpBoardings, tmpAlightings;
	double								tmpDepartureTime, tmpHeadway;
	string								tmpOut, tmpRouteId, tmpShapeId, tmpTripId, tmpDirection, tmpStopId;
	list<trip*>::iterator				tmpTripListIter;
	trip*								tmpTripPntr;

	ofstream outFile1;
	outFile1.open("ft_output_loadProfile.dat");
	outFile1 <<"routeId\tshapeId\ttripId\tdirection\tstopId\ttraveledDist\tdepartureTime\theadway\tdwellTime\tboardings\talightings\tload"<<endl;

	tmpNumStopTimes = 0;
	for(tmpTripListIter=tripList.begin();tmpTripListIter!=tripList.end();tmpTripListIter++){
		tmpTripPntr = (*tmpTripListIter);
		tmpMaxSeq = tmpTripPntr->getMaxSequence();
		tmpRouteId = tmpTripPntr->getRouteId();
		tmpShapeId = tmpTripPntr->getShapeId();
		tmpTripId = tmpTripPntr->getTripId();
		tmpDirection = tmpTripPntr->getDirection();
		for(i=1;i<=tmpMaxSeq;i++){
			tmpStopId = tmpTripPntr->getStop(i);
			tmpDepartureTime = tmpTripPntr->getSchDeparture(i);
			tmpHeadway = tmpTripPntr->getSchHeadway(i);
			tmpDwellTime = tmpTripPntr->getDwellTime(i);
			tmpBoardings = tmpTripPntr->getBoardings(i);
			tmpAlightings = tmpTripPntr->getAlightings(i);
			tmpVehLoad = tmpTripPntr->getVehLoad(i);
            outFile1 <<tmpRouteId.substr(1,99)<<"\t"<<tmpShapeId<<"\t"<<tmpTripId.substr(1,99)<<"\t"<<tmpDirection<<"\t"
                    <<tmpStopId.substr(1,99)<<"\t-1\t"<<tmpDepartureTime<<"\t"<<tmpHeadway<<"\t"<<tmpDwellTime<<"\t"<<tmpBoardings<<"\t"<<tmpAlightings<<"\t"<<tmpVehLoad<<endl;
            tmpNumStopTimes++;
            }
	}
	outFile1.close();
	return tmpNumStopTimes;
}
//////////////////////////////////////////////////////////////////////////////////////////////
int		printPassengerPaths(){
	//cout <<"***************************** PRINTING PATHS *****************************"<<endl;
	int									noOfPassengers, tmpMode, tmpStatus;
	double								tmpPAT;
	string								fileStr, tmpPassengerId, tmpOriginTaz, tmpDestinationTaz, tmpPath;
	map<string,passenger*>::iterator	tmpPassengerIter;
	passenger*							passengerPntr;

	noOfPassengers = 0;
	ofstream	outFile;
	outFile.open("ft_output_passengerPaths.dat");
	outFile <<"passengerId\tmode\toriginTaz\tdestinationTaz\tstartTime\tboardingStops\tboardingTrips\talightingStops\twalkingTimes"<<endl;
	for(tmpPassengerIter=passengerSet.begin();tmpPassengerIter!=passengerSet.end();tmpPassengerIter++){
		tmpPassengerId = (*tmpPassengerIter).first;
		passengerPntr = NULL;
		passengerPntr = passengerSet[tmpPassengerId];
		tmpMode = passengerPntr->getMode();
		tmpOriginTaz = passengerPntr->getOriginTAZ();
		tmpDestinationTaz = passengerPntr->getDestinationTAZ();
		tmpPAT = passengerPntr->getPAT();
		tmpPath = passengerPntr->getAssignedPath();
		tmpStatus = passengerPntr->getPassengerStatus();
		if(tmpPath!=""){
			outFile <<tmpPassengerId.substr(1,99)<<"\t"<<tmpMode<<"\t"<<tmpOriginTaz.substr(1,99)<<"\t"<<tmpDestinationTaz.substr(1,99)<<"\t"<<tmpPath<<endl;
			noOfPassengers++;
		}
	}
	outFile.close();
	return noOfPassengers;
}
//////////////////////////////////////////////////////////////////////////////////////////////
int		printPassengerTimes(){
	int									noOfPassengers, tmpStatus;
	double								tmpTravelCost;
	string								tmpExperiencedPath;
	list<passenger*>::iterator			tmpPassengerListIter;
	passenger*							tmpPassengerPntr;

	ofstream outFile2;
	outFile2.open("ft_output_passengerTimes.dat");
	outFile2 <<"passengerId\tmode\toriginTAz\tdestinationTaz\tstartTime\tendTime\tarrivalTimes\tboardingTimes\talightingTimes\ttravelCost"<<endl;

	noOfPassengers = 0;
	for(tmpPassengerListIter=passengerList.begin();tmpPassengerListIter!=passengerList.end();tmpPassengerListIter++){
		tmpPassengerPntr = NULL;
		tmpPassengerPntr = *tmpPassengerListIter;
		tmpStatus = tmpPassengerPntr->getPassengerStatus();
		if(tmpStatus==5){
			tmpExperiencedPath = tmpPassengerPntr->getExperiencedPath();
			tmpPassengerPntr->calculateExperiencedCost();
			tmpTravelCost = tmpPassengerPntr->getExperiencedCost();
			outFile2 <<tmpExperiencedPath.substr(1,99)<<"\t"<<floor(100*tmpTravelCost)/100.0<<endl;
			noOfPassengers++;
		}
	}
	outFile2.close();
	return noOfPassengers;
}
//////////////////////////////////////////////////////////////////////////////////////////////