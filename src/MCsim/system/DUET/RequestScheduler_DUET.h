#ifndef REQUESTSCHEDULER_DUET_H
#define REQUESTSCHEDULER_DUET_H

#include "../../src/RequestScheduler.h"

namespace MCsim
{
	class RequestScheduler_DUET: public RequestScheduler
	{
	private:
		bool roundRobin_Level = false;  // Switch to RR arbitration
		vector<unsigned int> requestorIndex;

	public:
		RequestScheduler_DUET(std::vector<RequestQueue*>&requestQueues, std::vector<CommandQueue*>& commandQueues, const std::map<unsigned int, bool>& requestorTable): 
			RequestScheduler(requestQueues,commandQueues, requestorTable) 
		{
			for(unsigned int index = 0; index < requestQueue.size(); index++) {
				requestorIndex.push_back(0);
			}			
		}

		void requestSchedule()
		{
			//cout<<"---------------------------------Inside Request Scheduler-----------------------------------------"<<endl;
			// Feed the Queues for the FR-FCFS 
			scheduledRequest_HP = NULL;
			scheduledRequest_RT = NULL;
			for(size_t index =0; index < requestQueue.size(); index++) 
			{
				//cout<<"1"<<endl;
				if(requestQueue[index]->isPerRequestor())
				{
					//cout<<"2"<<endl;
					if(requestQueue[index]->getQueueSize() > 0)
					{
						//cout<<"3"<<endl;
						for(unsigned int num = 0; num < 8; num++)
						{
							//cout<<"4"<<endl;
							scheduledRequest = NULL;
							if(requestQueue[index]->getSize(true,num) > 0) 
							{	
								//cout<<"5 and size of the request queue is   "<<requestQueue[index]->getSize(true,num)<<endl;
								scheduledRequest_HP = scheduleFR_BACKEND(index,num);
								if(scheduledRequest_HP != NULL){
									//cout<<"request is taken and the HP req size is  "<<scheduledRequest_HP->requestSize<<endl;	
								////	cout<<"6"<<endl;
									//cout<<"there is a request choosed to push for the HPMode from   "<<num<<endl;																									
									//updateRowTable(scheduledRequest->addressMap[Rank], scheduledRequest->addressMap[Bank], scheduledRequest->row); // Update the open row table for the device	
									//requestQueue[index]->removeRequest(); // Remove the request that has been choosed
								}	
							}
						}
					}
					scheduledRequest_HP = NULL;
				}
			}
			//cout<<"Now it is the RT turn"<<endl;							
			// Feed the Queues for the CMDBundle 
			for(size_t index =0; index < requestQueue.size(); index++) {
				//cout<<"7"<<endl;
				if(requestQueue[index]->isPerRequestor())
				{
					//cout<<"8"<<endl;
					if(requestQueue[index]->getQueueSize() > 0)
					{
						//cout<<"9"<<endl;
						for(unsigned int num=0; num<8; num++)
						{
							//cout<<"10"<<endl;
							scheduledRequest_RT = NULL;
							// requestorIndex[index]=num;   ENABLE if interested in having RR arbitration
							if(requestQueue[index]->getSize(true, num) > 0) 
							{					
								//cout<<"there is something to push RT  num  "<<num<<endl;										
								scheduledRequest_RT = requestQueue[index]->checkRequestIndex(num, 0); // Take the first request of the corresponding request queue
								//cout<<"request is taken and the req size is  "<<scheduledRequest_RT->requestSize<<endl;	
								// Determine if the request target an open row or not
								//cout<<"id hast   "<<scheduledRequest_RT->requestorID<<endl;
								if(isSchedulable(scheduledRequest_RT, requestQueue[0]->isRowHit(scheduledRequest_RT), true))
								{	
									//cout<<"there is a "<<endl;	
									//cout<<"  RT  8  and reqIndex is  "<<num<<"   index is   "<<0<<endl;
									scheduledRequest_RT = requestQueue[index]->getRequest(num, 0, true);																			
									//updateRowTable(scheduledRequest->addressMap[Rank], scheduledRequest->addressMap[Bank], scheduledRequest->row); // Update the open row table for the device										
									//requestQueue[index]->removeRequest(); // Remove the request that has been choosed
								}
							}
							scheduledRequest_RT = NULL;
						}
					}
					scheduledRequest_RT = NULL;
				}
			}
		}
	};
}

#endif  /* REQUESTSCHEDULER_DIRECT_H */


