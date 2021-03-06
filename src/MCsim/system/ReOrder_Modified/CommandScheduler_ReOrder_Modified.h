
#include "../../src/CommandScheduler.h"

namespace MCsim
{
	class CommandScheduler_ReOrder_Modified: public CommandScheduler
	{
	private:
		vector<std::pair<BusPacket*, unsigned int>> commandRegisters;
		vector<bool> servedFlags;
		vector<bool> queuePending;
		vector<bool> virChannelRegister; 
		vector<pair<BusPacket*, unsigned int>> srtFIFO;

		unsigned int indexCAS;
		unsigned int indexACT;
		unsigned int indexPRE;
		unsigned int roundType;
						
		std::map<unsigned int, unsigned int> virMap;

	public:
		CommandScheduler_ReOrder_Modified(vector<CommandQueue*>& commandQueues, const map<unsigned int, bool>& requestorTable):
			CommandScheduler(commandQueues, requestorTable)
		{
			roundType = BusPacketType::RD;	// Read Type
			indexCAS = 0;
			indexACT = 0;
			indexPRE = 0;
			for(unsigned index=0; index < commandQueue.size(); index++) {
				queuePending.push_back(false);
			}
			for(unsigned index=0; index < commandQueue.size(); index++) {
				servedFlags.push_back(false);
			}
		}

		BusPacket* commandSchedule()
		{
			scheduledCommand = NULL;
			for(unsigned int index = 0; index < commandQueue.size(); index++) { 			// Scan the command queue for ready CAS
				checkCommand = NULL;
				if(commandQueue[index]->getSize(true) > 0 && queuePending[index] == false) {
					checkCommand = commandQueue[index]->getCommand(true);
				}
				if(checkCommand!=NULL) {
					if(checkCommand->busPacketType == RD  && checkCommand->requestorID ==0){
						
					}
					if(isReady(checkCommand, index)) {
						
						commandRegisters.push_back(make_pair(checkCommand,checkCommand->bank));						
						queuePending[index] = true;	
					}
				}
			}

			bool CAS_remain = false;
			bool newRound = true;	// Should reset servedFlag?
			//bool newType = true;	// Should switch casType?
			bool switch_expected = false;	// A different CAS can be schedule
			//bool haveCAS = false;
			
			if(!commandRegisters.empty()) 
			{
				
				for(unsigned int index =0; index < commandRegisters.size(); index++){
					checkCommand = commandRegisters[index].first;
					if(checkCommand->busPacketType < ACT) {
						bool isServed = true;
						isServed = servedFlags[commandRegisters[index].second];																				
						if(isServed == false && checkCommand->busPacketType == roundType){	
				
							CAS_remain = true;						
							break;
						}
						else if (isServed == false && checkCommand->busPacketType != roundType)
						{
							switch_expected = true;
							//break;
						}
					}
				}
				if(CAS_remain == true){
					newRound = false;
				}
				else
				{
					if(switch_expected == true)
						newRound = true;					
				}
				
				if(newRound) { 
					
					for(unsigned int index = 0; index < servedFlags.size(); index++) {
						servedFlags[index] = false;
					}
					if(switch_expected == true)
					{
						
						if(roundType == WR) {roundType = RD;}
						else {roundType = WR;}
					}	
				}
				
				checkCommand = NULL;	
			}	
		
			// Schedule the FIFO with CAS blocking
			unsigned int registerIndex = 0;
			if(!commandRegisters.empty()) {
				for(unsigned int index = 0; index < commandRegisters.size(); index++) {
					checkCommand = commandRegisters[index].first;
					if(requestorCriticalTable.at(checkCommand->requestorID) == false) {
						if(servedFlags[virMap[checkCommand->requestorID]] == false && checkCommand->busPacketType == roundType) {
							if(isIssuable(checkCommand)) {
								scheduledCommand = checkCommand;
								registerIndex = index;
								break;
							}							
						}
					}
					else {
						// Not served and same CAS type
						if(servedFlags[checkCommand->bank] == false && checkCommand->busPacketType == roundType) {
							if(isIssuable(checkCommand)) {
								scheduledCommand = checkCommand;
								registerIndex = index;
								break;
							}
						}						
					}
					checkCommand = NULL;
				}				
			}
			// Round-Robin ACT arbitration
			if(scheduledCommand == NULL && !commandRegisters.empty()) {
				for(unsigned int index = 0; index < commandRegisters.size(); index++) {
					if(commandRegisters[index].first->busPacketType == ACT) {
						checkCommand = commandRegisters[index].first;
						if(isIssuable(checkCommand)) {
							scheduledCommand = checkCommand;
							registerIndex = index;
							break;
						}
						checkCommand = NULL;
					}
				}
			}
			// Round-Robin PRE arbitration
			if(scheduledCommand == NULL && !commandRegisters.empty()) {
				for(unsigned int index = 0; index < commandRegisters.size(); index++) {
					if(commandRegisters[index].first->busPacketType == PRE) {
						checkCommand = commandRegisters[index].first;
						if(isIssuable(checkCommand)) {
							scheduledCommand = checkCommand;
							registerIndex = index;
							break;
						}
					}
				}
			}
			if(scheduledCommand != NULL) {
				if(scheduledCommand->busPacketType < ACT) {
					if(requestorCriticalTable.at(scheduledCommand->requestorID) == false) {
						servedFlags[virMap[scheduledCommand->requestorID]] = true;
					}
					else {
						servedFlags[scheduledCommand->bank] = true;
					}
				}
				queuePending[commandRegisters[registerIndex].second] = false;
				sendCommand(scheduledCommand, commandRegisters[registerIndex].second, false);
				commandRegisters.erase(commandRegisters.begin()+registerIndex);
			}
			checkCommand = NULL;	
			return scheduledCommand;
		}
	};
}
