#include <iostream>
#include <algorithm>
#include "CommandScheduler.h"
#include "MemoryDevice.h"
#include "global.h"
using namespace std;

using namespace MCsim;

CommandScheduler::CommandScheduler(
	vector<CommandQueue*>& commandQueues, const map<unsigned int, bool>& requestorTable,vector<CommandQueue*>& commandQueue_RT):
commandQueue(commandQueues),
commandQueue_RT(commandQueue_RT),
requestorCriticalTable(requestorTable)
{
	// If the command queue is used as per memory level
	cmdQueueTimer = std::map<unsigned int, std::map<unsigned int, unsigned int> >();
	// If the command queue is used as per requestor queue
	for(unsigned int index=0; index<commandQueue.size(); index++) {
		reqCmdQueueTimer.push_back(std::map<unsigned int, std::map<unsigned int, unsigned int> >());
	}

	memoryDevice = NULL;
	refreshMachine = NULL;
	scheduledCommand = NULL;
	checkCommand = NULL;
	clock = 1;
	ranks = 0;
	banks = 0;
}
CommandScheduler::~CommandScheduler()
{
	cmdQueueTimer.clear();	
}
void CommandScheduler::connectMemoryDevice(MemoryDevice* memDev)
{
	memoryDevice = memDev;
	ranks = memoryDevice->get_Rank();
	banks = memoryDevice->get_Bank();
	refreshMachine = new RefreshMachine(commandQueue, ranks, banks, getTiming("tREFI"), getTiming("tRFCpb"), getTiming("tRFCab"), getTiming("tRFC"),getTiming("tRCD"));
}

unsigned int CommandScheduler::getTiming(const string& param) // Accsess the Ramulator backend to determine the timing specifications
{
	return memoryDevice->get_constraints(param);
}

bool CommandScheduler::refreshing() // Perform refresh
{
	return refreshMachine->refreshing();
}
void CommandScheduler::updateRowTable_cmd(unsigned rank, unsigned bank, unsigned row)
{
	bankTable_cmd[rank][bank] = row;
}
bool CommandScheduler::isRowHit_cmd(BusPacket* cmd)
{
	
	bool isHit = false;
	if(bankTable_cmd.find(cmd->rank) != bankTable_cmd.end()) {
		if(bankTable_cmd[cmd->rank].find(cmd->bank) != bankTable_cmd[cmd->rank].end()) {
			if(bankTable_cmd[cmd->rank][cmd->bank] == cmd->row){ 
				isHit = true; 
			}
		}	
	}	
	return isHit;
}
void CommandScheduler::refresh() // Indicator of reaching refresh interval
{
	BusPacket* tempCmd = NULL;
	if(refreshMachine->reachInterval())
	{
		refreshMachine->refresh(tempCmd);
		if(tempCmd != NULL) {
			if(isIssuableRefresh(tempCmd)) {
				if(tempCmd->busPacketType == PRE)						
				{
					cout<<"TRACE-COMMAND:PRE"<<"\t"<<clock<<":"<<"\t\tAddress: "<<tempCmd->address<<"\t\tBank: "<<tempCmd->bank<<"\t\tColumn: "<<tempCmd->column<<"\tRow: "<<tempCmd->row<<endl;										
				}
				else if(tempCmd->busPacketType == REF)
				{
					cout<<"TRACE-COMMAND:REF"<<"\t"<<clock<<":"<<"\t\tAddress: "<<tempCmd->address<<"\t\tBank: "<<tempCmd->bank<<"\t\tColumn: "<<tempCmd->column<<"\tRow: "<<tempCmd->row<<endl;		
				}
				else if(tempCmd->busPacketType == REFPB)
				{				
					cout<<"TRACE-COMMAND:REFPB"<<"\t"<<clock<<":"<<"\t\tAddress: "<<tempCmd->address<<"\t\tBank: "<<tempCmd->bank<<"\t\tColumn: "<<tempCmd->column<<"\tRow: "<<tempCmd->row<<endl;		
				}
				else if(tempCmd->busPacketType == PREA)
				{
				 	cout<<"TRACE-COMMAND:PREA"<<"\t"<<clock<<":"<<"\t\tAddress: "<<tempCmd->address<<"\t\tBank: "<<tempCmd->bank<<"\t\tColumn: "<<tempCmd->column<<"\tRow: "<<tempCmd->row<<endl;		
				}	
				else if(tempCmd->busPacketType == ACT)
				{
					if(refreshMachine->busyBank(tempCmd->bank) == false)
				 		cout<<"TRACE-COMMAND:ACT"<<"\t"<<clock<<":"<<"\t\tAddress: "<<tempCmd->address<<"\t\tBank: "<<tempCmd->bank<<"\t\tColumn: "<<tempCmd->column<<"\tRow: "<<tempCmd->row<<endl;		
				}					
				memoryDevice->receiveFromBus(tempCmd);
				refreshMachine->popCommand();				
			}	
		}
	tempCmd = NULL;
	delete tempCmd;
	}	
}

bool CommandScheduler::isReady(BusPacket* cmd, unsigned int index) // Each command queue share the same command ready time
{
	if(commandQueue[index]->isPerRequestor()) {
		if(reqCmdQueueTimer[index].find(cmd->requestorID) == reqCmdQueueTimer[index].end()) {
			reqCmdQueueTimer[index][cmd->requestorID] = map<unsigned int, unsigned int>();
		}
		if(reqCmdQueueTimer[index][cmd->requestorID].find(cmd->busPacketType) != reqCmdQueueTimer[index][cmd->requestorID].end())
		{
			if(reqCmdQueueTimer[index][cmd->requestorID][cmd->busPacketType] != 0)
			{
				return false;
			}
		}
	}
	else 
	{
		if(cmdQueueTimer.find(index) == cmdQueueTimer.end()) {
			cmdQueueTimer[index] = map<unsigned int, unsigned int>();
		}
		if(cmdQueueTimer[index].find(cmd->busPacketType) != cmdQueueTimer[index].end()) {		
			if(cmdQueueTimer[index][cmd->busPacketType] != 1){ // changed for the dual mode requirement	
				return false;
			}
		}		
	}
	return true;
}
unsigned int CommandScheduler::isReadyTimer(BusPacket* cmd, unsigned int index) 
{
	if(cmdQueueTimer.find(index) == cmdQueueTimer.end()) {
		cmdQueueTimer[index] = map<unsigned int, unsigned int>();
	}
	if(cmdQueueTimer[index].find(cmd->busPacketType) != cmdQueueTimer[index].end()) {
			return cmdQueueTimer[index][cmd->busPacketType];		
	}
	return 0;			
}
bool CommandScheduler::isReadyTimerCmd(BusPacketType cmd, unsigned int index, bool mode) 
{
	if(mode) 
	{
		for(unsigned int indexx=0; indexx<commandQueue.size(); indexx++) {
			if(indexx != index) 
			{
				if(cmdQueueTimer.find(index) == cmdQueueTimer.end()) {
					cmdQueueTimer[index] = map<unsigned int, unsigned int>();
				}
				if(cmdQueueTimer[index].find(cmd) != cmdQueueTimer[index].end()) {
						if(cmdQueueTimer[index][cmd] == 0) 
							return true;		
				}			
			}
		}	
		return false;	
	}
	else
	{		
		if(cmdQueueTimer.find(index) == cmdQueueTimer.end()) {
			cmdQueueTimer[index] = map<unsigned int, unsigned int>();
		}
		if(cmdQueueTimer[index].find(cmd) != cmdQueueTimer[index].end()) {
				if(cmdQueueTimer[index][cmd] == 0) 
					return true;		
		}					
	}		
}
bool CommandScheduler::isIssuable(BusPacket* cmd) // Check if the command is issueable on the channel
{
	return memoryDevice->command_check(cmd);
}
bool CommandScheduler::isIssuableRefresh(BusPacket* cmd) 
{		
	return memoryDevice->command_check(cmd);
}
void CommandScheduler::sendCommand(BusPacket* cmd, unsigned int index, bool bypass) // Send the actual command to the device
{
	
	// Update command counter
	if(commandQueue[index]->isPerRequestor()) 
	{		
		for(unsigned int type = RD; type != PDE; type++) {
			reqCmdQueueTimer[index][cmd->requestorID][type] = std::max(reqCmdQueueTimer[index][cmd->requestorID][type], 
			memoryDevice->command_timing(cmd, static_cast<BusPacketType>(type)));
		}
		// For the command trace option
		/*
		if(cmd->busPacketType == PRE)						
		{
			if(cmd->address > 999999){
				cout<<"TRACE-COMMAND:PRE"<<"\t"<<clock<<":"<<"\t\tAddress: "<<cmd->address<<"\tBank: "<<cmd->bank<<"\t\tColumn: "<<cmd->column<<"\tRow: "<<cmd->row<<endl;		}
			else {
				cout<<"TRACE-COMMAND:PRE"<<"\t"<<clock<<":"<<"\t\tAddress: "<<cmd->address<<"\t\tBank: "<<cmd->bank<<"\t\tColumn: "<<cmd->column<<"\tRow: "<<cmd->row<<endl;		}										}
		else if(cmd->busPacketType == RD)
		{						
			if(cmd->address > 999999){
				cout<<"TRACE-COMMAND:RD"<<"\t"<<clock<<":"<<"\t\tAddress: "<<cmd->address<<"\tBank: "<<cmd->bank<<"\t\tColumn: "<<cmd->column<<"\tRow: "<<cmd->row<<endl;		}										
			else { 
				cout<<"TRACE-COMMAND:RD"<<"\t"<<clock<<":"<<"\t\tAddress: "<<cmd->address<<"\t\tBank: "<<cmd->bank<<"\t\tColumn: "<<cmd->column<<"\tRow: "<<cmd->row<<endl;	}			
		}
		else if(cmd->busPacketType == WR){
			if(cmd->address > 999999){
				cout<<"TRACE-COMMAND:WR"<<"\t"<<clock<<":"<<"\t\tAddress: "<<cmd->address<<"\tBank: "<<cmd->bank<<"\t\tColumn: "<<cmd->column<<"\tRow: "<<cmd->row<<endl;												}
			else {
				cout<<"TRACE-COMMAND:WR"<<"\t"<<clock<<":"<<"\t\tAddress: "<<cmd->address<<"\t\tBank: "<<cmd->bank<<"\t\tColumn: "<<cmd->column<<"\tRow: "<<cmd->row<<endl;	}									
		}
		else if(cmd->busPacketType == WRA){
			if(cmd->address > 999999){
				cout<<"TRACE-COMMAND:WRA"<<"\t"<<clock<<":"<<"\t\tAddress: "<<cmd->address<<"\tBank: "<<cmd->bank<<"\t\tColumn: "<<cmd->column<<"\tRow: "<<cmd->row<<endl;												}
			else {
				cout<<"TRACE-COMMAND:WRA"<<"\t"<<clock<<":"<<"\t\tAddress: "<<cmd->address<<"\t\tBank: "<<cmd->bank<<"\t\tColumn: "<<cmd->column<<"\tRow: "<<cmd->row<<endl;	}									
		}
		else if(cmd->busPacketType == RDA){
			if(cmd->address > 999999){
				cout<<"TRACE-COMMAND:RDA"<<"\t"<<clock<<":"<<"\t\tAddress: "<<cmd->address<<"\tBank: "<<cmd->bank<<"\t\tColumn: "<<cmd->column<<"\tRow: "<<cmd->row<<endl;												}
			else {
				cout<<"TRACE-COMMAND:RDA"<<"\t"<<clock<<":"<<"\t\tAddress: "<<cmd->address<<"\t\tBank: "<<cmd->bank<<"\t\tColumn: "<<cmd->column<<"\tRow: "<<cmd->row<<endl;	}									
		}
		else if(cmd->busPacketType == ACT || cmd->busPacketType == ACT_R || cmd->busPacketType == ACT_W){
			if(cmd->address > 999999){
				cout<<"TRACE-COMMAND:ACT"<<"\t"<<clock<<":"<<"\t\tAddress: "<<cmd->address<<"\tBank: "<<cmd->bank<<"\t\tColumn: "<<cmd->column<<"\tRow: "<<cmd->row<<endl;		}
			else {
				cout<<"TRACE-COMMAND:ACT"<<"\t"<<clock<<":"<<"\t\tAddress: "<<cmd->address<<"\t\tBank: "<<cmd->bank<<"\t\tColumn: "<<cmd->column<<"\tRow: "<<cmd->row<<endl;	}								
		}
		*/
		
		
		//commandQueue[index]->removeCommand(cmd->requestorID);
		if(cmd->busPacketType > WRA)
		{
			
			// if(commandQueue[index]->getRequestorSize(cmd->requestorID,false) > 0)
			
			if(cmd->address == commandQueue[index]->getRequestorCommand(cmd->requestorID,false)->address && cmd->busPacketType == commandQueue[index]->getRequestorCommand(cmd->requestorID,false)->busPacketType && cmd->requestorID == commandQueue[index]->getRequestorCommand(cmd->requestorID,false)->requestorID)
			{
			
				commandQueue[index]->removeCommand(cmd->requestorID,false);	
			}
			
			if(cmd->address == commandQueue_RT[index]->getRequestorCommand(cmd->requestorID,true)->address && cmd->busPacketType == commandQueue_RT[index]->getRequestorCommand(cmd->requestorID,true)->busPacketType && cmd->requestorID == commandQueue_RT[index]->getRequestorCommand(cmd->requestorID,true)->requestorID){
			
				commandQueue_RT[index]->removeCommand(cmd->requestorID,true);	

			}
			
		}
	}
	else {
		if(!bypass)
		{
			for(unsigned int type = RD; type != PDE; type++) {
				cmdQueueTimer[index][type] = std::max(cmdQueueTimer[index][type], 
					memoryDevice->command_timing(cmd, static_cast<BusPacketType>(type)));
					
			}	
			if(cmd->busPacketType > WRA){
				if(cmd->address == commandQueue[index]->getCommand(true)->address && cmd->busPacketType ==commandQueue[index]->getCommand(true)->busPacketType )
					commandQueue[index]->removeCommand();	
			
				if(cmd->address == commandQueue_RT[index]->getCommand(true)->address && cmd->busPacketType ==commandQueue_RT[index]->getCommand(true)->busPacketType)
					commandQueue_RT[index]->removeCommand();	
			}
		}
		else
		{
			for(unsigned int type = RD; type != PDE; type++) {
				cmdQueueTimer[index][type] = std::max(cmdQueueTimer[index][type], 
					memoryDevice->command_timing(cmd, static_cast<BusPacketType>(type)));
			}
		}
		/*
		For the command trace option
		
		if(cmd->busPacketType == PRE)						
		{
			if(cmd->address > 999999){
				cout<<"TRACE-COMMAND:PRE"<<"\t"<<clock<<":"<<"\t\tAddress: "<<cmd->address<<"\tBank: "<<cmd->bank<<"\t\tColumn: "<<cmd->column<<"\tRow: "<<cmd->row<<endl;		}
			else {
				cout<<"TRACE-COMMAND:PRE"<<"\t"<<clock<<":"<<"\t\tAddress: "<<cmd->address<<"\t\tBank: "<<cmd->bank<<"\t\tColumn: "<<cmd->column<<"\tRow: "<<cmd->row<<endl;	}															
		}
		else if(cmd->busPacketType == RD){
			if(cmd->address > 999999){
				cout<<"TRACE-COMMAND:RD"<<"\t"<<clock<<":"<<"\t\tAddress: "<<cmd->address<<"\tBank: "<<cmd->bank<<"\t\tColumn: "<<cmd->column<<"\tRow: "<<cmd->row<<endl;		
			}										
			else { 
				cout<<"TRACE-COMMAND:RD"<<"\t"<<clock<<":"<<"\t\tAddress: "<<cmd->address<<"\t\tBank: "<<cmd->bank<<"\t\tColumn: "<<cmd->column<<"\tRow: "<<cmd->row<<endl;	}	
		}
		else if(cmd->busPacketType == WR){
			if(cmd->address > 999999){
				cout<<"TRACE-COMMAND:WR"<<"\t"<<clock<<":"<<"\t\tAddress: "<<cmd->address<<"\tBank: "<<cmd->bank<<"\t\tColumn: "<<cmd->column<<"\tRow: "<<cmd->row<<endl;												}
			else {
				cout<<"TRACE-COMMAND:WR"<<"\t"<<clock<<":"<<"\t\tAddress: "<<cmd->address<<"\t\tBank: "<<cmd->bank<<"\t\tColumn: "<<cmd->column<<"\tRow: "<<cmd->row<<endl;	}	
		}
		else if(cmd->busPacketType == WRA){
			if(cmd->address > 999999){
				cout<<"TRACE-COMMAND:WRA"<<"\t"<<clock<<":"<<"\t\tAddress: "<<cmd->address<<"\tBank: "<<cmd->bank<<"\t\tColumn: "<<cmd->column<<"\tRow: "<<cmd->row<<endl;												}
			else {
				cout<<"TRACE-COMMAND:WRA"<<"\t"<<clock<<":"<<"\t\tAddress: "<<cmd->address<<"\t\tBank: "<<cmd->bank<<"\t\tColumn: "<<cmd->column<<"\tRow: "<<cmd->row<<endl;	}									
		}
		else if(cmd->busPacketType == RDA){
			if(cmd->address > 999999){
				cout<<"TRACE-COMMAND:RDA"<<"\t"<<clock<<":"<<"\t\tAddress: "<<cmd->address<<"\tBank: "<<cmd->bank<<"\t\tColumn: "<<cmd->column<<"\tRow: "<<cmd->row<<endl;												}
			else {
				cout<<"TRACE-COMMAND:RDA"<<"\t"<<clock<<":"<<"\t\tAddress: "<<cmd->address<<"\t\tBank: "<<cmd->bank<<"\t\tColumn: "<<cmd->column<<"\tRow: "<<cmd->row<<endl;	}									
		}
		else if(cmd->busPacketType == ACT || cmd->busPacketType == ACT_R || cmd->busPacketType == ACT_W){
			if(cmd->address > 999999){
				cout<<"TRACE-COMMAND:ACT"<<"\t"<<clock<<":"<<"\t\tAddress: "<<cmd->address<<"\tBank: "<<cmd->bank<<"\t\tColumn: "<<cmd->column<<"\tRow: "<<cmd->row<<endl;		}
			else {
				cout<<"TRACE-COMMAND:ACT"<<"\t"<<clock<<":"<<"\t\tAddress: "<<cmd->address<<"\t\tBank: "<<cmd->bank<<"\t\tColumn: "<<cmd->column<<"\tRow: "<<cmd->row<<endl;	}						
		}
		*/
			
	}
	
	memoryDevice->receiveFromBus(scheduledCommand);
}
void CommandScheduler::commandClear()
{
	delete scheduledCommand;
	scheduledCommand = NULL;
}

void CommandScheduler::tick()
{
	// Countdown command ready timer
	for(unsigned int index = 0; index < commandQueue.size(); index++) {
		if(commandQueue[index]->isPerRequestor()) {
			for(auto &requestor : reqCmdQueueTimer[index]) {
				for(auto &counter : reqCmdQueueTimer[index][requestor.first]) {
					if(counter.second > 0) { 
						counter.second--;
					}
				}
			}
		}
		else
		{
			if(!cmdQueueTimer[index].empty()) {
				for(auto &counter : cmdQueueTimer[index]) {
					if(counter.second > 1) { // changed for the dual mode requirement	
						counter.second--; 
					}
				}
			}			
		}
	}
	refreshMachine->step();
	
	clock++;
}
