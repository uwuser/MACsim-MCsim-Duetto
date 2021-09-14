#ifndef COMMANDGENERATOR_H
#define COMMANDGENERATOR_H

#include <queue>
#include "Request.h"
#include "BusPacket.h"
#include "CommandQueue.h"

namespace MCsim
{
	class CommandGenerator
	{
	public:
		CommandGenerator(unsigned int dataBus,std::vector<CommandQueue*>& commandQueues,std::vector<CommandQueue*>& commandQueues_RT);
		virtual ~CommandGenerator();
		virtual bool commandGenerate(Request* request, bool open, bool mode) = 0;

			BusPacket* getCommand();
		BusPacket* getCommand_RT();
		void removeCommand();
		void removeCommand_RT();
		unsigned int bufferSize();
		unsigned int bufferSize_RT();

	protected:
		unsigned int dataBusSize;
		std::vector<CommandQueue*>& commandQueue;
		std::vector<CommandQueue*>& commandQueue_RT;
		bool first[8] = {true,true,true,true,true,true,true,true};
		bool first_RT[8] = {true,true,true,true,true,true,true,true};
		std::map<unsigned int, std::pair<unsigned int, unsigned int>>lookupTable;
		std::queue< BusPacket* > commandBuffer;
		std::queue< BusPacket* > commandBuffer_RT;
	};
}

#endif /* COMMANDGENERATOR_H */