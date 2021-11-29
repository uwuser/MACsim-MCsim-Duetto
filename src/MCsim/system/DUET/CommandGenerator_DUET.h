#ifndef COMMANDGENERATOR_DUET_H
#define COMMANDGENERATOR_DUET_H

#include "../../src/CommandGenerator.h"
namespace MCsim
{
	class CommandGenerator_DUET : public CommandGenerator
	{
	public:
		CommandGenerator_DUET(unsigned int dataBus, std::vector<CommandQueue *> &commandQueues, vector<CommandQueue *> &commandQueue_RT) : CommandGenerator(dataBus, commandQueues, commandQueue_RT) {}

		bool commandGenerate(Request *request, bool open, bool mode)
		{
			BusPacketType CAS = RD;
			if (request->requestType == DATA_WRITE)
			{
				CAS = WR;
			}
			unsigned size = 1; // request->requestSize/dataBusSize;
			unsigned id = request->requestorID;
			uint64_t address = request->address;
			unsigned rank = 0; //request->rank; // 0 *****;
			unsigned bank = request->bank;
			unsigned row = request->row;
			unsigned col = request->col;
			unsigned sa = request->subArray;

			/**				
				The command should be generated based on the type of the request and 
				state of the bank.
				@param mode defines if the command should be generated for RTA or HPA.							
			*/
			if (!mode)
			{
				/**				
					DuoMC command generator only pushes the commands when the particular command 
					queue is empty. But note that the queue structure is per bank per requestor. 
					Therefore for each pair of requestor and bank, there exists a command queues for HP.  											
				*/
				if (commandQueue[bank]->getRequestorSize(id, false) == 0)
				{
					if (!open)
					{
						commandBuffer.push(new BusPacket(PRE, id, address, col, row, bank, rank, sa, NULL, 0));
						commandBuffer.push(new BusPacket(ACT, id, address, col, row, bank, rank, sa, NULL, 0));
					}
					for (unsigned int x = 0; x < size; x++)
					{
						commandBuffer.push(new BusPacket(CAS, id, address, col + x, row, bank, rank, sa, request->data, 0));
					}
				}
				else if (commandQueue[bank]->getRequestorSize(id, false) > 0)
				{
					return false;
				}
				return true;
			}
			else
			{
				/**				
					DuoMC command generator only pushes the commands when the particular command 
					queue is empty. But note that the queue structure is per bank per requestor. 
					Therefore for each pair of requestor and bank, there exists a command queues for RT.  											
				*/
				if (commandQueue_RT[bank]->getRequestorSize(id, true) == 0)
				{
					if (!open)
					{
						commandBuffer_RT.push(new BusPacket(PRE, id, address, col, row, bank, rank, sa, NULL, 0));
						commandBuffer_RT.push(new BusPacket(ACT, id, address, col, row, bank, rank, sa, NULL, 0));
					}
					for (unsigned int x = 0; x < size; x++)
					{
						commandBuffer_RT.push(new BusPacket(CAS, id, address, col + x, row, bank, rank, sa, request->data, 0));
					}
				}
				else if (commandQueue_RT[bank]->getRequestorSize(id, true) > 0)
				{
					return false;
				}
				return true;
			}
		}
	};
}

#endif /* COMMANDGENERATOR_OPEN_H*/