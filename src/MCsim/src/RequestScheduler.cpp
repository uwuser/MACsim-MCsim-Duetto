#include <math.h>
#include "RequestScheduler.h"
#include "CommandGenerator.h"
#include "global.h"

using namespace MCsim;

RequestScheduler::RequestScheduler(std::vector<RequestQueue*>&requestQueues, std::vector<CommandQueue*>& commandQueues, const std::map<unsigned int, bool>& requestorTable):
	requestorCriticalTable(requestorTable),
	requestQueue(requestQueues),
	commandQueue(commandQueues)
{
	commandGenerator = NULL;
	scheduledRequest = NULL;
	req1 = NULL;
	req2 = NULL;
	id = 0;
	clockCycle = 1;
}

RequestScheduler::~RequestScheduler()
{
	//bankTable.clear();
}

void RequestScheduler::connectCommandGenerator(CommandGenerator *generator)
{
	commandGenerator = generator;
}

void RequestScheduler::flushWriteReq(bool sw)
{ 
	switch_enable =  sw;
}
bool RequestScheduler::serviceWrite(int qIndex)
{
	if(requestQueue[qIndex]->isWriteEnable() && requestQueue[qIndex]->getSize(false, 0) == 0)
		return true;
	else
		return false;	
}
bool RequestScheduler::writeEnable(int qIndex)
{
	return requestQueue[qIndex]->isWriteEnable();
}
unsigned int RequestScheduler::bufferSize(unsigned int qIndex){
	return requestQueue[qIndex]->getSize(false, 0);
}

Request* RequestScheduler::scheduleFR_BACKEND(unsigned int qIndex, unsigned int reqIndex)
{
	//cout<<"4"<<endl;
	for(unsigned int index=0; index < requestQueue[qIndex]->getSize(true, reqIndex); index++) {		
		//cout<<"20"<<endl;
	
		if(requestQueue[0]->isRowHit(requestQueue[qIndex]->checkRequestIndex(reqIndex,index))){
			//cout<<"21"<<endl;
			if(isSchedulable(requestQueue[qIndex]->checkRequestIndex(reqIndex,index), requestQueue[0]->isRowHit(requestQueue[qIndex]->checkRequestIndex(reqIndex,index)),0)){
				//cout<<"################################  for queue  "<<reqIndex<<endl;
				//cout<<"^^^^^^^^^^^^^^^^^^^^^  "<<requestQueue[qIndex]->checkRequestIndex(reqIndex,index)->address<<"    "<<index<<endl;
				//cout<<"||||||||||||||||||for the request   "<<requestQueue[qIndex]->checkRequestIndex(reqIndex,index)->address<<"   index is   "<<index<<endl;			
				return requestQueue[qIndex]->getRequest(reqIndex,index, false);
			}			
		}	
	}
	//cout<<"5"<<endl;
	for(unsigned int index=0; index < requestQueue[qIndex]->getSize(true, reqIndex); index++) {
		//cout<<"22"<<endl;
		if(isSchedulable(requestQueue[qIndex]->checkRequestIndex(reqIndex,index), requestQueue[0]->isRowHit(requestQueue[qIndex]->checkRequestIndex(reqIndex,index)),0)){
			//cout<<"8  and reqIndex is  "<<reqIndex<<"   index is   "<<index<<endl;
			return requestQueue[qIndex]->getRequest(reqIndex,index, false);
		}
	}	
	//cout<<"6"<<endl;
	return NULL;
}
bool RequestScheduler::isSchedulable(Request* request, bool open, bool mode)
{
	if(request->requestType == DATA_READ)						
	{
		if(request->address > 999999){
			TRACE_REQ("TRACE-REQUEST:READ"<<"\t\t"<<clockCycle<<":"<<"\t\tAddress: "<<request->address<<"\tBank: "<<request->bank<<"\t\tColumn: "<<request->col<<"\t\tRow: "<<request->row);										}
		else {
			TRACE_REQ("TRACE-REQUEST:READ"<<"\t\t"<<clockCycle<<":"<<"\t\tAddress: "<<request->address<<"\t\tBank: "<<request->bank<<"\t\tColumn: "<<request->col<<"\t\tRow: "<<request->row);}
	}
	else if(request->requestType == DATA_WRITE)	{
		if(request->address > 999999){
			TRACE_REQ("TRACE-REQUEST:WRITE"<<"\t\t"<<clockCycle<<":"<<"\t\tAddress: "<<request->address<<"\tBank: "<<request->bank<<"\t\tColumn: "<<request->col<<"\t\tRow: "<<request->row);}
		else {
			 TRACE_REQ("TRACE-REQUEST:WRITE"<<"\t\t"<<clockCycle<<":"<<"\t\tAddress: "<<request->address<<"\t\tBank: "<<request->bank<<"\t\tColumn: "<<request->col<<"\t\tRow: "<<request->row);}								
	}
	//cout<<"what is request size here?  "<<request->requestSize<<endl;
	return commandGenerator->commandGenerate(request, open, mode);
}


void RequestScheduler::step()
{
	clockCycle++;
}
