#include "RequestQueue.h"
#include "CommandGenerator.h"
#include "global.h"

using namespace MCsim;

//#define DEBUG(str) std::cerr<< str <<std::endl;

RequestQueue::RequestQueue(bool perRequestor, bool writeQueueEnable):
	writeQueueEnable(writeQueueEnable),
	perRequestorEnable(perRequestor)
{
	flag = false;
	if(writeQueueEnable) { // Low and high watermark must be chosen according to the design specifications 		
		writeQueue = new WriteQueue(5,20);
	} 
	else {
		writeQueue = NULL;
	}
	generalBuffer = std::vector< Request* >();
	requestorBuffer = std::map<unsigned int, std::vector<Request*>>();
}

RequestQueue::~RequestQueue()
{
	delete writeQueue;
	for(auto it=generalBuffer.begin(); it!=generalBuffer.end(); it++) {
		delete (*it);
	}
	generalBuffer.clear();
	for(auto it=requestorBuffer.begin(); it!=requestorBuffer.end(); it++) {
		for(auto req=it->second.begin(); req!=it->second.end(); req++) {
			delete (*req);
		}
		it->second.clear();
	}
	requestorBuffer.clear();
	bankTable.clear();
}

bool RequestQueue::isWriteEnable()
{
	return writeQueueEnable;
}

bool RequestQueue::isPerRequestor()
{
	return perRequestorEnable;
}

// Add request based on criticality
bool RequestQueue::insertRequest(Request* request)
{	
	
	if(writeQueueEnable && request->requestType == DATA_WRITE) {
		writeQueue->insertWrite(request);
		return true;
	}

	if(perRequestorEnable) {
		if(requestorBuffer.find(request->requestorID) == requestorBuffer.end()) {
	
			requestorBuffer[0] = std::vector<Request*>();
			requestorBuffer[1] = std::vector<Request*>();
			requestorBuffer[2] = std::vector<Request*>();
			requestorBuffer[3] = std::vector<Request*>();
			requestorBuffer[4] = std::vector<Request*>();
			requestorBuffer[5] = std::vector<Request*>();
			requestorBuffer[6] = std::vector<Request*>();
			for(int i; i<8; i++){
				requestorOrder.push_back(i);
			}
		}
		requestorBuffer[request->requestorID].push_back(request);
	}
	else {		
		generalBuffer.push_back(request);
	}
	
	return true;
}
// How many requestor share the same queue
unsigned int RequestQueue::getQueueSize()
{
	return requestorOrder.size();
}

unsigned int RequestQueue::getSize(bool requestor, unsigned int index)
{
	
	if(requestor) {
		if(requestorOrder.size() == 0) {
	
			return 0; 
		}
		else {
		
			// No such requestor
			//if(requestorBuffer.find(requestorOrder[index]) == requestorBuffer.end()){ 
			if(requestorBuffer.find(index) == requestorBuffer.end()){ 
		
				return 0; 
			}	
			else { 
		
				return requestorBuffer[index].size();
			}
		}
	}
	else 
	{ 
		return generalBuffer.size();
	}
}
void RequestQueue::setWriteMode(bool set)
{
	writeMode = set;
}
bool RequestQueue::isRowHit(Request* request)
{
	
	bool isHit = false;
	if(bankTable.find(request->addressMap[Rank]) != bankTable.end()) {
		if(bankTable[request->rank].find(request->addressMap[Bank]) != bankTable[request->addressMap[Rank]].end()) {
			if(bankTable[request->addressMap[Rank]][request->addressMap[Bank]] == request->row){ 
				isHit = true; 
			}
		}	
	}	
	return isHit;
}
bool RequestQueue::isWriteMode()
{
	return writeMode;
}
unsigned int RequestQueue::generalReadBufferSize(bool requestor)
{
	return generalBuffer.size();
}
// If care about the fairness
Request* RequestQueue::getRequest(unsigned int reqIndex, unsigned int index, bool mode)
{
	// Scan the requestorqueue by index, instead of requestorID value 
	if(!mode)
	{
		
		//if(requestorBuffer[requestorOrder[reqIndex]].empty()) {
		if(requestorBuffer[reqIndex].empty()) {
			return NULL;
		}
		else {
		
			scheduledRequest_HP[reqIndex].first = true;
			//scheduledRequest_HP[reqIndex].second = std::make_pair(requestorOrder[reqIndex],index);
			scheduledRequest_HP[reqIndex].second = std::make_pair(reqIndex,index);
			return requestorBuffer[reqIndex][index];
			//return requestorBuffer[requestorOrder[reqIndex]][index];
		}
	}
	else
	{
		
		//if(requestorBuffer[requestorOrder[reqIndex]].empty()) {
		if(requestorBuffer[reqIndex].empty()) {
			return NULL;
		}
		else {
		
			scheduledRequest_RT[reqIndex].first = true;
			//scheduledRequest_RT[reqIndex].second = std::make_pair(requestorOrder[reqIndex],index);
			scheduledRequest_RT[reqIndex].second = std::make_pair(reqIndex,index);
			//return requestorBuffer[requestorOrder[reqIndex]][index];
			return requestorBuffer[reqIndex][index];
		}
	}
}

// Scan the requestorqueue by index, instead of requestorID value 
Request* RequestQueue::checkRequestIndex(unsigned int reqIndex, unsigned int index)
{
	//if(requestorBuffer[requestorOrder[reqIndex]].empty()) {
	if(requestorBuffer[reqIndex].empty()) {
		return NULL;
	}
	else {
		//return requestorBuffer[requestorOrder[reqIndex]][index];
		return requestorBuffer[reqIndex][index];
	}
}
void RequestQueue::syncRequest(unsigned int reqIndex, unsigned int index)
{
	
	scheduledRequest_HP[reqIndex].first =true;	
	scheduledRequest_HP[reqIndex].second = scheduledRequest_RT[reqIndex].second;
}
// Check wether the general buffer is empty 
bool RequestQueue::isEmpty()
{
	if(generalBuffer.empty()) {
		return true;
	}
	return false;
}

// If does not care about the fairness
Request* RequestQueue::getRequest(unsigned int index)
{
	if(generalBuffer.empty()) {
		return NULL;
	}
	else {
		scheduledRequest.first = false;
		scheduledRequest.second = std::make_pair(0,index);
		return generalBuffer[index];		
	}
}
// Take the request without removing from the buffer - per requestor
Request* RequestQueue::getRequestCheck(unsigned int index)
{
	if(generalBuffer.empty()) {
		return NULL;
	}
	else {
		return generalBuffer[index];		
	}
}
bool RequestQueue::switchMode()
{
	if(flag)
	{			
		if(writeQueue->bufferSize() == 0){
			flag = false;
			return true;
		}
		return false;
	}
	else
	{
		if(writeQueue->highWatermark())
		{
			flag = true;
			return false;
		}	
		return true;
	}	
	return true;
}

unsigned int RequestQueue::writeSize()
{
	return writeQueue->bufferSize();
}

Request* RequestQueue::scheduleWritecheck()
{
	return writeQueue->getWrite(0);
}

void RequestQueue::removeWriteRequest()
{
	writeQueue->removeWrite(0);
}
// Find the earliest request per requestor per bank
Request* RequestQueue::earliestperBankperReq(unsigned int p, unsigned int b)
{
	Request* temp_init = NULL;
	Request* temp_sec = NULL;
	Request* temp = NULL;
	// search in the buffer to find latest req from p and b
	if(generalBuffer.empty()) {
		return NULL;
	}
	else 
	{
		for(unsigned int k = 0; k < generalBuffer.size() ; k++)
		{
			temp_init = generalBuffer[k];
			if(temp_init->bank == b && temp_init->requestorID == p)
			{
				temp = temp_init;
				break;	
			}
		}
		for(unsigned int l = 0; l < generalBuffer.size() ; l++)
		{			
			temp_sec = generalBuffer[l];
			if(temp_sec->bank == b && temp_sec->requestorID == p)
			{
				if(temp_sec->arriveTime < temp->arriveTime)
				{
					temp = temp_sec;
				}				
			}
		}
		return temp;
	}
}
// Remove the previously access request, Once a request is buffered to another location, remove from the request queue
void RequestQueue::removeRequest(bool mode, unsigned int reqID)
{
	
	if(!mode)
	{
		
		unsigned id = scheduledRequest_HP[reqID].second.first;
		
		unsigned index = scheduledRequest_HP[reqID].second.second;
		
		if(scheduledRequest_HP[reqID].first) {
		
			requestorBuffer[id].erase(requestorBuffer[id].begin() + index);
		}
		else {
		
			generalBuffer.erase(generalBuffer.begin() + index);
		}
		
	}
	else
	{
		
		unsigned id = scheduledRequest_RT[reqID].second.first;
		unsigned index = scheduledRequest_RT[reqID].second.second;
		if(scheduledRequest_RT[reqID].first) {
			requestorBuffer[id].erase(requestorBuffer[id].begin() + index);
		}
		else {
			generalBuffer.erase(generalBuffer.begin() + index);
		}
	}			
}

void RequestQueue::updateRowTable(unsigned rank, unsigned bank, unsigned row)
{
	
	
	bankTable[rank][bank] = row;
}
unsigned int RequestQueue::returnRowTable(unsigned rank, unsigned bank)
{	
	return bankTable[rank][bank];
}










