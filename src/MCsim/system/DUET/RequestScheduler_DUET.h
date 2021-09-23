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
			// Feed the Queues for the FR-FCFS 
			scheduledRequest_HP = NULL;
			scheduledRequest_RT = NULL;
			for(size_t index =0; index < requestQueue.size(); index++) 
			{
				if(requestQueue[index]->isPerRequestor())
				{
					if(requestQueue[index]->getQueueSize() > 0)
					{
						for(unsigned int num = 0; num < 8; num++)
						{
							scheduledRequest = NULL;
							if(requestQueue[index]->getSize(true,num) > 0) 
							{	
								scheduledRequest_HP = scheduleFR_BACKEND(index,num);
								if(scheduledRequest_HP != NULL){
									//cout<<"there is a request choosed to push for the HPMode from   "<<num<<endl;		
								}	
							}
						}
					}
					scheduledRequest_HP = NULL;
				}
			}						
			// Feed the Queues for the CMDBundle 
			for(size_t index =0; index < requestQueue.size(); index++) {
				if(requestQueue[index]->isPerRequestor())
				{
					if(requestQueue[index]->getQueueSize() > 0)
					{
						for(unsigned int num=0; num<8; num++)
						{
							scheduledRequest_RT = NULL;
							// requestorIndex[index]=num;   ENABLE if interested in having RR arbitration
							if(requestQueue[index]->getSize(true, num) > 0) 
							{															
								scheduledRequest_RT = requestQueue[index]->checkRequestIndex(num, 0); // Take the first request of the corresponding request queue
					
								if(isSchedulable(scheduledRequest_RT, requestQueue[0]->isRowHit(scheduledRequest_RT), true))
								{	
									scheduledRequest_RT = requestQueue[index]->getRequest(num, 0, true);					
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


