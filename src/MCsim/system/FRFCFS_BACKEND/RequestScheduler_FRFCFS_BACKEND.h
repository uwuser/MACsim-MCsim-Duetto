#ifndef REQUESTSCHEDULER_FRFCFS_BACKEND_H
#define REQUESTSCHEDULER_FRFCFS_BACKEND_H

#include "../../src/RequestScheduler.h"

namespace MCsim
{
	class RequestScheduler_FRFCFS_BACKEND: public RequestScheduler
	{
	private:
		bool roundRobin_Level = false;  // Switch to RR arbitration
		vector<unsigned int> requestorIndex;

	public:
		RequestScheduler_FRFCFS_BACKEND(std::vector<RequestQueue*>&requestQueues, std::vector<CommandQueue*>& commandQueues, const std::map<unsigned int, bool>& requestorTable): 
			RequestScheduler(requestQueues, commandQueues, requestorTable) 
		{
			for(unsigned int index = 0; index < requestQueue.size(); index++) {
				requestorIndex.push_back(0);
			}
		}

		void requestSchedule()
		{
			
			for(size_t index =0; index < requestQueue.size(); index++) 
			{
				if(requestQueue[index]->isPerRequestor())
				{
					if(requestQueue[index]->getQueueSize() > 0)
					{
			
						for(unsigned int num = 0; num < requestQueue[index]->getQueueSize(); num++)
						{
			
							scheduledRequest = NULL;
							if(requestQueue[index]->getSize(true,num) > 0) 
							{	
			
								scheduledRequest = scheduleFR_BACKEND(index,num);
			
								if(scheduledRequest != NULL){
									// Determine if the request target an open row or not																											
									updateRowTable(scheduledRequest->addressMap[Rank], scheduledRequest->addressMap[Bank], scheduledRequest->row); // Update the open row table for the device	
			
									requestQueue[index]->removeRequest(); // Remove the request that has been choosed
								}																	
							
								
							}
						}
					}
					scheduledRequest = NULL;
				}
			}			
		}
	};
}

#endif  /* REQUESTSCHEDULER_DIRECT_H */


