#ifndef COMMANDQUEUE_H
#define COMMANDQUEUE_H

#include <queue>
#include <map>
#include "BusPacket.h"

namespace MCsim
{
	class CommandQueue
	{
	public:
		CommandQueue(bool perRequestor);
		virtual ~CommandQueue();
		// Insert cmd based on either general criticality or requestorID
		bool insertCommand(BusPacket* command, bool critical, bool mode);
		bool ACTdiff;
		bool isPerRequestor();
		// Check size of buffer
		unsigned int getRequestorIndex();
		unsigned int getRequestorSize(unsigned int index, bool mode);
		unsigned int getSize(bool critical);

		// Access the cmd on top of each cmd queue
		BusPacket* getRequestorCommand(unsigned int index, bool mode);
		BusPacket* getRequestorCommand_position(unsigned int index, unsigned int p, bool mode);
		BusPacket* getCommand(bool critical);
		BusPacket* checkCommand(bool critical, unsigned int index);

		// Remove most recently accessed cmd (from requestor queue or general fifo)
		void removeCommand(unsigned int requestorID);
		void removeCommand();
		void setACT(unsigned int x);
		
	private:
		// Indicate if perRequestor buffer is enabled
		bool perRequestorEnable;
		// Indicate the last accessed queue : critical or nonCritical
		bool scheduledQueue;
		bool requestorQueue;
		unsigned int requestorIndex;
		// Order of requestorID in the buffer map; used to determine the requestor scheduling order and find in the bufferMap
		std::vector<unsigned int> requestorMap;
		// Dynamic allocate buffer to individual requestors that share the same resource level
		std::map< unsigned int, std::vector<BusPacket*> > requestorBuffer;
		std::map< unsigned int, std::vector<BusPacket*> > requestorBuffer_RT;
		// General buffer for critical cmds
		std::vector<BusPacket*> hrtBuffer;
		// General buffer for nonCritical cmds
		std::vector<BusPacket*> srtBuffer;
	};
}

#endif /* COMMANDQUEUE_H */
