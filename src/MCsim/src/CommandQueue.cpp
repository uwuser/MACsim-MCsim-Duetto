#include <iostream>
#include "CommandQueue.h"
#include "global.h"
#include <algorithm>   
#include "CommandScheduler.h"
using namespace std;

using namespace MCsim;

CommandQueue::CommandQueue(bool perRequestor) // ctor
{
	scheduledQueue = true;
	requestorQueue = false;
	requestorIndex = 0;
	ACTdiff = false;
	perRequestorEnable = perRequestor;
}

CommandQueue::~CommandQueue()  //dtor
{
	std::vector<BusPacket*> empty;
	std::swap(hrtBuffer, empty);
	std::swap(srtBuffer, empty);
}

bool CommandQueue::isPerRequestor() // determine if the command queue structure is per requestor not
{
	return perRequestorEnable;
}

unsigned int CommandQueue::getRequestorIndex()  // return the size command queue 
{
	return requestorMap.size();
}

unsigned int CommandQueue::getRequestorSize(unsigned int index, bool mode)  // Get the size of individual buffer cmd for each requestor
{
	if(index > requestorMap.size()) {
		DEBUG("COMMAND QUEUE: ACCESSED QUEUE OS OUT OF RANGE");
		//cout<<"15 the size is  "<<requestorMap.size()<<endl;
		abort();
	}
	else if(!mode) {
		
		if(requestorBuffer.find(index) == requestorBuffer.end()) {
			requestorBuffer[index] = std::vector<BusPacket*>();
			requestorMap.push_back(index);
		}
		
		return requestorBuffer[requestorMap[index]].size(); 
	}
	else if(mode)
	{
		if(requestorBuffer_RT.find(index) == requestorBuffer_RT.end()) {
			requestorBuffer_RT[index] = std::vector<BusPacket*>();
			requestorMap.push_back(index);
		}
		
		return requestorBuffer_RT[requestorMap[index]].size(); 
	}
	return 0;
}
 
unsigned int CommandQueue::getSize(bool critical) // Get the size of the general cmd buffers based on the criticality
{

	if(critical){
		return hrtBuffer.size(); 
	}	
	else{
		return srtBuffer.size(); 
	}
}

bool CommandQueue::insertCommand(BusPacket* command, bool critical, bool mode) // Inserting the cmd to the corresponding queues according to the queuing structure
{
	if(!mode)
	{
		if(perRequestorEnable){
			if(requestorBuffer.find(command->requestorID) == requestorBuffer.end()) {
				requestorBuffer[command->requestorID] = std::vector<BusPacket*>();
				requestorMap.push_back(command->requestorID);
			}
			requestorBuffer[command->requestorID].push_back(command);
		}
		else {
			if(critical)  
				hrtBuffer.push_back(command); 		
			else 
				srtBuffer.push_back(command); 		
		}
		return true;
	}
	else
	{
		if(perRequestorEnable){
			if(requestorBuffer_RT.find(command->requestorID) == requestorBuffer_RT.end()) {
				requestorBuffer_RT[command->requestorID] = std::vector<BusPacket*>();
				requestorMap.push_back(command->requestorID);
			}
			requestorBuffer_RT[command->requestorID].push_back(command);
		}
		else {
			if(critical)  
				hrtBuffer.push_back(command); 		
			else 
				srtBuffer.push_back(command); 		
		}
		return true;
	}
}
BusPacket* CommandQueue::getCommand(bool critical) // Get a cmd from either general cmd buffers
{
	scheduledQueue = critical;
	if(critical && !hrtBuffer.empty()) { 
		return hrtBuffer.front(); 
	}
	else if(!critical && !srtBuffer.empty()) { 
		return srtBuffer.front(); 
	}
	else {
		DEBUG("COMMAND QUEUE: CHECK THE SIZE OF getCommand");
		abort();
		return NULL; 
	}
}

BusPacket* CommandQueue::checkCommand(bool critical, unsigned int index) // Check a cmd from general buffers while for the purpose of checking
{
	if(critical && !hrtBuffer.empty()) { 
		return hrtBuffer[index]; 
	}
	else if(!critical && !srtBuffer.empty()) { 
		return srtBuffer[index]; 
	}
	else {
		DEBUG("COMMAND QUEUE: CHECK THE SIZE OF checkCommand");
		abort();
		return NULL; 
	}
}

BusPacket* CommandQueue::getRequestorCommand(unsigned int index, bool mode) // Get a cmd from a particular requestor cmd buffer
{
	if(!mode){
		return requestorBuffer[requestorMap[index]].front();
	}
	else 
	{
		return requestorBuffer_RT[requestorMap[index]].front();
	}
	
}
void CommandQueue::removeCommand(unsigned int requestorID) // Remove a cmd from the beginning of the specific requestor queue
{
	requestorBuffer[requestorID].erase(requestorBuffer[requestorID].begin());
}

void CommandQueue::removeCommand() // Remove a cmd from the general buffers (hrt/srt) 
{
	if(scheduledQueue == true) {
		hrtBuffer.erase(hrtBuffer.begin());
	}
	else {
		srtBuffer.erase(srtBuffer.begin());
	}
}
BusPacket* CommandQueue::getRequestorCommand_position(unsigned int index, unsigned int p, bool mode) // Get a cmd from a particular requestor cmd buffer
{
	if(!mode)
	{
		return requestorBuffer[requestorMap[index]].at(p);
	}
	else
	{
		return requestorBuffer_RT[requestorMap[index]].at(p);
	}
	
}
void CommandQueue::setACT(unsigned int x) // Determine if the scheduler require different ACT for read/write (ACT_R and ACT_W)
{
	if(x == 1)
		ACTdiff = true;
	else
		ACTdiff = false;	
}

