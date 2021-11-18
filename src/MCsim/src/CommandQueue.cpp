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
	if(!mode) 
	{
		if(requestorMap.size() == 0) 
		{
			requestorBuffer[0] = std::vector<BusPacket*>();
			requestorBuffer[1] = std::vector<BusPacket*>();
			requestorBuffer[2] = std::vector<BusPacket*>();
			requestorBuffer[3] = std::vector<BusPacket*>();
			requestorBuffer[4] = std::vector<BusPacket*>();
			requestorBuffer[5] = std::vector<BusPacket*>();
			requestorBuffer[6] = std::vector<BusPacket*>();
			requestorBuffer[7] = std::vector<BusPacket*>();
			requestorBuffer_RT[0] = std::vector<BusPacket*>();
			requestorBuffer_RT[1] = std::vector<BusPacket*>();
			requestorBuffer_RT[2] = std::vector<BusPacket*>();
			requestorBuffer_RT[3] = std::vector<BusPacket*>();
			requestorBuffer_RT[4] = std::vector<BusPacket*>();
			requestorBuffer_RT[5] = std::vector<BusPacket*>();
			requestorBuffer_RT[6] = std::vector<BusPacket*>();
			requestorBuffer_RT[7] = std::vector<BusPacket*>();
			for(int i; i<8; i++){
				requestorMap.push_back(i);
			}
		}
		return requestorBuffer[requestorMap[index]].size(); 
	}
	else if(mode)
	{
		
		if(requestorMap.size() == 0) 
		{
		
			requestorBuffer[0] = std::vector<BusPacket*>();
			requestorBuffer[1] = std::vector<BusPacket*>();
			requestorBuffer[2] = std::vector<BusPacket*>();
			requestorBuffer[3] = std::vector<BusPacket*>();
			requestorBuffer[4] = std::vector<BusPacket*>();
			requestorBuffer[5] = std::vector<BusPacket*>();
			requestorBuffer[6] = std::vector<BusPacket*>();
			requestorBuffer[7] = std::vector<BusPacket*>();
			requestorBuffer_RT[0] = std::vector<BusPacket*>();
			requestorBuffer_RT[1] = std::vector<BusPacket*>();
			requestorBuffer_RT[2] = std::vector<BusPacket*>();
			requestorBuffer_RT[3] = std::vector<BusPacket*>();
			requestorBuffer_RT[4] = std::vector<BusPacket*>();
			requestorBuffer_RT[5] = std::vector<BusPacket*>();
			requestorBuffer_RT[6] = std::vector<BusPacket*>();
			requestorBuffer_RT[7] = std::vector<BusPacket*>();
			for(int i; i<8; i++){
				requestorMap.push_back(i);
			}
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
		
		return requestorBuffer[index].front();
	}
	else 
	{
		
		return requestorBuffer_RT[index].front();
	}
	
}
void CommandQueue::removeCommand(unsigned int requestorID,bool mode) // Remove a cmd from the beginning of the specific requestor queue
{
	if(!mode)
	{
		
		requestorBuffer[requestorID].erase(requestorBuffer[requestorID].begin());		
		
	}
	else
	{

		requestorBuffer_RT[requestorID].erase(requestorBuffer_RT[requestorID].begin());

	}	

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

