#ifndef COMMANDGENERATOR_DUET_H
#define COMMANDGENERATOR_DUET_H

#include "../../src/CommandGenerator.h"
namespace MCsim
{
	class CommandGenerator_DUET: public CommandGenerator
	{
	public:
		CommandGenerator_DUET(unsigned int dataBus,std::vector<CommandQueue*>& commandQueues,vector<CommandQueue*>& commandQueue_RT):
			CommandGenerator(dataBus,commandQueues,commandQueue_RT) {}			

		bool commandGenerate(Request* request, bool open, bool mode)
		{			
			BusPacketType CAS = RD;
			if(request->requestType == DATA_WRITE) {
				CAS = WR;
			}
			unsigned size = 1;// request->requestSize/dataBusSize; 
			unsigned id = request->requestorID;
			unsigned long long address = request->address;
			unsigned rank = request->rank; // 0 *****;
			unsigned bank = request->bank;
			unsigned row = request->row;
			unsigned col = request->col;	
			unsigned sa = request->subArray;	
			if(!mode)
			{
				//cout<<" ---------------------------------inside command generator and pushing to the HP queues"<<endl;
				if(commandQueue[id]->getSize(true) == 0)
				{			
					if(!open) { // Assuming the initial state of the banks in device is idle
					
						commandBuffer.push(new BusPacket(PRE, id, address, col, row, bank, rank, sa, NULL, 0));
						commandBuffer.push(new BusPacket(ACT, id, address, col, row, bank, rank, sa, NULL, 0));
					}
					for(unsigned int x = 0; x < size; x++) {
				
						commandBuffer.push(new BusPacket(CAS, id, address, col+x, row, bank, rank, sa, request->data, 0));
					}
				}
				else if	(commandQueue[id]->getSize(true) > 0)
				{		
					return false;
				}
				return true;
			}
			else
			{
				if(commandQueue_RT[id]->getSize(true) == 0)
				{	
					if(!open) { // Assuming the initial state of the banks in device is idle
					
						commandBuffer_RT.push(new BusPacket(PRE, id, address, col, row, bank, rank, sa, NULL, 0));
						commandBuffer_RT.push(new BusPacket(ACT, id, address, col, row, bank, rank, sa, NULL, 0));
					}
					for(unsigned int x = 0; x < size; x++) {
			
						commandBuffer_RT.push(new BusPacket(CAS, id, address, col+x, row, bank, rank, sa, request->data, 0));
					}
				}
				else if	(commandQueue_RT[id]->getSize(true) > 0)
				{			
					return false;
				}
				return true;
			}
			
		}
	};
}

#endif /* COMMANDGENERATOR_OPEN_H*/