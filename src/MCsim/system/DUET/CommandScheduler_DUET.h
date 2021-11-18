
#ifndef COMMANDSCHEDULER_DUET_H
#define COMMANDSCHEDULER_DUET_H

#include "../../src/CommandScheduler.h"

namespace MCsim
{
	class CommandScheduler_DUET : public CommandScheduler
	{
	private:
		vector<RequestQueue *> &requestQueues_local;
		unsigned int registerIndex;
		unsigned int temp;
		string mode;	  //RTMode, HPMode
		string mode_prev; //RTMode, HPMode
		bool start;
		unsigned long int hpa_count;
		unsigned long int rta_count;
		vector<std::pair<BusPacket *, unsigned int>> commandRegisters;
		vector<bool> servedFlags;
		vector<bool> queuePending;
		vector<bool> queuePendingShared;
		unsigned long long req_wcl_index;
		vector<bool> virChannelRegister;
		vector<bool> order_flag; //to keep track of the orders
		vector<unsigned int> Order;
		vector<unsigned int> Order_temp;
		vector<bool> sharedBank;
		unsigned int active_shared_req;
		unsigned int active_shared_bank;
		bool active_shared_exist;
		unsigned int bank_count;
		unsigned int REQ_count;
		unsigned int k;
		bool deadline_set_flag[8];
		bool req_open[8];
		bool req_statues_flag[8];
		std::map<unsigned int, unsigned int> service_deadline;
		std::map<unsigned int, unsigned int> deadline_track;
		unsigned long long total_read;
		unsigned long long total_write;
		std::map<unsigned int, unsigned int> temp_timer;
		unsigned int init_deadline;
		unsigned int init_deadline_CR;
		unsigned int init_deadline_CW;
		unsigned int init_deadline_OR;
		unsigned int init_deadline_OW;
		unsigned int indexCAS;
		unsigned int indexACT;
		unsigned int indexPRE;
		unsigned long long sum_wcl;
		unsigned long long count;
		unsigned int roundType;
		unsigned int CAStimer;
		bool flag_deadline;
		unsigned int RR;
		unsigned int RR_RT;
		unsigned int tCCD;
		unsigned int tWL;
		unsigned int tRL;
		unsigned int tBUS;
		unsigned int tWR;
		unsigned int tRTW;
		unsigned int tRTP;
		unsigned int tWTR;
		unsigned int tRRD;
		unsigned int tWtoR;
		unsigned int tRCD;
		unsigned int tRP;
		unsigned int wcl;
		int N;
		string impl;
		int suspect_requestor;
		bool suspect_flag;
		int front_has_pre;
		int front_has_act;
		int front_has_rd;
		int front_has_wr;
		bool remove;
		bool debug;

	public:
		CommandScheduler_DUET(vector<RequestQueue *> &requestQueues, vector<CommandQueue *> &commandQueues,
							  const map<unsigned int, bool> &requestorTable, vector<CommandQueue *> &commandQueue_RT) : CommandScheduler(commandQueues, requestorTable, commandQueue_RT),
																														requestQueues_local(requestQueues)
		{
			
			roundType = BusPacketType::RD; // Read Type
			active_shared_req = 1000;
			active_shared_bank = 1000;
			active_shared_exist = false;
			indexCAS = 0;
			indexACT = 0;
			indexPRE = 0;
			CAStimer = 4;
			front_has_pre = 0;
			front_has_act = 0;
			front_has_rd = 0;
			front_has_wr = 0;
			debug = true;
			tCCD = 4;
			total_read = 0;
			REQ_count = 8; /***** SHOULD  BE CONFIGURED *****/
			sum_wcl = 0;
			count = 0;
			total_write = 0;
			suspect_flag = false;
			N = 0;
			bank_count = 8;		  /***** SHOULD  BE CONFIGURED *****/
			init_deadline = 150;  /***** SHOULD  BE CONFIGURED *****/
			init_deadline_CR = 150;
			init_deadline_CW = 150;
			init_deadline_OR = 150;
			init_deadline_OW = 150;

			mode  = "HPA";
			start = true;
			impl  =  "C"; 
			//sharedBank.push_back(7);  					 /***** SHOULD  BE CONFIGURED *****/
			//sharedBank.push_back();  						 /***** SHOULD  BE CONFIGURED *****/
			//sharedBank.push_back();  						 /***** SHOULD  BE CONFIGURED *****/
			flag_deadline = false;
			for (unsigned int i = 0; i < REQ_count; i++)
				deadline_set_flag[i] = false;
			for (unsigned int i = 0; i < REQ_count; i++)
				req_open[i] = false;
			for (unsigned int i = 0; i < REQ_count; i++)
				req_statues_flag[i] = false;
			for (unsigned index = 0; index < commandQueue.size(); index++)
				queuePendingShared.push_back(false);
			for (unsigned index = 0; index < REQ_count; index++)
				service_deadline[index] = init_deadline;
			for (unsigned index = 0; index < REQ_count; index++)
				queuePending.push_back(false);
			for (unsigned index = 0; index < REQ_count; index++)
				servedFlags.push_back(false);
			for (unsigned index = 0; index < REQ_count; index++)			
				order_flag.push_back(false);
			
		}
		BusPacket *returnOldest(unsigned int reqID) 
		{
			// return the oldest command of the requestor reqID
			oldest_1 = NULL;
			oldest_2 = NULL;

			for (unsigned int i = 0; i < commandQueue_RT.size(); i++)
			{
				if (commandQueue_RT[i]->getRequestorIndex() > 0) // There is more than 0 requestor in the design
				{
					if (commandQueue_RT[i]->getRequestorSize(reqID, true) > 0) // Return the buffer size of the requestor
					{
						oldest_1 = commandQueue_RT[i]->getRequestorCommand(reqID, true);
						if (oldest_1 != NULL)
							break;
					}
				}
			}
			if (oldest_1 != NULL)
			{
				for (unsigned int i = 0; i < commandQueue_RT.size(); i++)
				{
					if (commandQueue_RT[i]->getRequestorIndex() > 0) // There is more than 0 requestor in the design
					{
						if (commandQueue_RT[i]->getRequestorSize(reqID, true) > 0) // Return the buffer size of the requestor
						{
							oldest_2 = commandQueue_RT[i]->getRequestorCommand(reqID, true);
							if (oldest_2 != NULL)
							{
								if (oldest_2->arriveTime < oldest_1->arriveTime)
								{
									oldest_1 = oldest_2;
								}
							}
							oldest_2 = NULL;
						}
					}
				}
			}

			return oldest_1;
		}
		bool reqNotEmpty(unsigned int reqID)
		{
			// return true if at least one command exists for reqID 
			for (unsigned int i = 0; i < commandQueue_RT.size(); i++)
			{
				if (commandQueue_RT[i]->getRequestorIndex() > 0) // There is more than 0 requestor in the design
				{
					if (commandQueue_RT[i]->getRequestorSize(reqID, true) > 0) // Return the buffer size of the requestor
					{
						return true;
					}
				}
			}

			return false;
		}
		bool isBlocked(BusPacket *cmd)
		{
			//  if there is another oldest request to the same bank
			for (unsigned int loop = 0; loop < Order.size(); loop++)
			{
				if (Order.at(loop) != cmd->requestorID)
				{
					if (returnOldest(Order.at(loop))->bank == cmd->bank)
						return true;
				}
			}
			return false;
		}
		bool isShared(unsigned int index)
		{
			for (int i = 0; i < sharedBank.size(); i++)
			{
				if (sharedBank.at(i) == index)
				{
					abort();
					return true;
				}
			}
			return false;
		}
		void setDeadline()
		{
			// Set the deadline to each requestor in the system - this represents the deadline for the oldest request of each requestor
			// It works only on the RT command queue obviously

			bool order_fix = false;
			for (unsigned int index = 0; index < commandQueue_RT.size(); index++)
			{
				for (unsigned int num = 0; num < commandQueue_RT[index]->getRequestorIndex(); num++) // For all requestors from "num". getRequestorIndex() gives the number of requestors
				{
					if (commandQueue_RT[index]->getRequestorSize(num, true) > 0) // Return the buffer size of the requestor
					{
						if (deadline_set_flag[num] == false)
						{							
							///////////////
							if(clock != 1) 
							{
						
								if(commandQueue_RT[index]->getRequestorCommand(num, true)->busPacketType == PRE) 
								{
						
									if (commandQueue_RT[index]->getRequestorCommand_position(num, 2, true)->busPacketType == RD) 
									{
						
										service_deadline[num] = init_deadline_CR;
									}
									else if (commandQueue_RT[index]->getRequestorCommand_position(num, 2, true)->busPacketType == WR) 
									{
									
										service_deadline[num] = init_deadline_CW;
									}
								}
								else
								{
									
									if (commandQueue_RT[index]->getRequestorCommand(num, true)->busPacketType == RD) 
									{
									
										service_deadline[num] = init_deadline_OR;
									}
									else if (commandQueue_RT[index]->getRequestorCommand(num, true)->busPacketType == WR) 
									{
									
										service_deadline[num] = init_deadline_OW;
									}
								}
							}
							else {
								
								service_deadline[num] = init_deadline;
							}
							
							//////////////
							
							deadline_track[num] = clock;
							deadline_set_flag[num] = true;
							Order.push_back(num);
						}
					}
				}
			}
			for (unsigned int num = 0; num < REQ_count; num++) 
			{
				for (unsigned int index = 0; index < commandQueue_RT.size(); index++)
				{
					if (commandQueue_RT[index]->getRequestorSize(num, true) > 0)
						break;
					if (index == (commandQueue_RT.size() - 1))
						service_deadline[num] = init_deadline;
				}
			}
		}
		unsigned int WCLator(unsigned int requestor, BusPacket *cmd)
		{
			//cout<<"Schedule"<<endl;
			/***** DDR3 1600 H *****/
			unsigned int LACT[16] = {9, 15, 21, 27, 34, 40, 46, 52, 59, 65, 71, 77, 84, 90, 96, 102};
			unsigned int LPRE[16] = {2, 3, 6, 7, 9, 11, 13, 14, 17, 18, 19, 22, 23, 26, 27, 29};

			front_has_pre = 0;
			front_has_act = 0;
			front_has_rd = 0;
			front_has_wr = 0;
		
			Order_temp = Order;
			N = 0;

			for (unsigned int i = 0; i < Order.size(); i++) // number of requestors that are higher priority than "requestor"
			{
				if (Order.at(i) == requestor)
					N = i;
			}

			
			if(impl == "C") {
				if (cmd != NULL) // number of requestors that are higher priority than "requestor"  -- if the cmd is CAS, it changes the order, then we track updated Order
				{

					if (cmd->busPacketType == RD || cmd->busPacketType == WR)
					{
						for (unsigned int h = 0; h < Order_temp.size(); h++)
						{
							if (Order_temp.at(h) == cmd->requestorID)
							{
								Order_temp.erase(Order_temp.begin() + h);
								Order_temp.push_back(cmd->requestorID);
								break;
							}
						}
						for (unsigned int i = 0; i < Order_temp.size(); i++)
						{
							if (Order_temp.at(i) == requestor)
								N = i; // number of requestors that are ahead of this specific requestor
						}
					}
				}
			}

			for (unsigned int traverse = 0; traverse < N; traverse++) // number of requestors that are higher priority than "requestor" and has PRE
			{
				oldest_3 = NULL;
				oldest_3 = returnOldest(Order_temp.at(traverse));
				if (oldest_3 != NULL)
				{
					if (oldest_3->busPacketType == PRE)
						front_has_pre++;
				}
			}
			for (unsigned int traverse = 0; traverse < N; traverse++) // number of requestors that are higher priority than "requestor" and has ACT
			{
				oldest_3 = NULL;
				oldest_3 = returnOldest(Order_temp.at(traverse));
				if (oldest_3 != NULL)
				{
					if (oldest_3->busPacketType == ACT)
						front_has_act++;
					else if (oldest_3->busPacketType == PRE)
					{
						if (commandQueue_RT[oldest_3->bank]->getRequestorCommand_position(oldest_3->requestorID, 1, true)->busPacketType == ACT)
							front_has_act++;
					}
				}
			}
			for (unsigned int traverse = 0; traverse < N; traverse++) // number of requestors that are higher priority than "requestor" and has RD
			{
				oldest_3 = NULL;
				oldest_3 = returnOldest(Order_temp.at(traverse));
				if (oldest_3 != NULL)
				{
					if (oldest_3->busPacketType == RD)
						front_has_rd++;
					else if (oldest_3->busPacketType == ACT)
					{
						if (commandQueue_RT[oldest_3->bank]->getRequestorCommand_position(oldest_3->requestorID, 1, true)->busPacketType == RD)
							front_has_rd++;
					}
					else if (oldest_3->busPacketType == PRE)
					{
						if (commandQueue_RT[oldest_3->bank]->getRequestorCommand_position(oldest_3->requestorID, 2, true)->busPacketType == RD)
							front_has_rd++;
					}
				}
			}
			for (unsigned int traverse = 0; traverse < N; traverse++) // number of requestors that are higher priority than "requestor" and has WR
			{
				oldest_3 = NULL;
				oldest_3 = returnOldest(Order_temp.at(traverse));
				if (oldest_3 != NULL)
				{
					if (oldest_3->busPacketType == WR)
						front_has_wr++;
					else if (oldest_3->busPacketType == ACT)
					{
						if (commandQueue_RT[oldest_3->bank]->getRequestorCommand_position(oldest_3->requestorID, 1, true)->busPacketType == WR)
							front_has_wr++;
					}
					else if (oldest_3->busPacketType == PRE)
					{
						if (commandQueue_RT[oldest_3->bank]->getRequestorCommand_position(oldest_3->requestorID, 2, true)->busPacketType == WR)
							front_has_wr++;
					}
				}
			}
			
			if (N - 1 < 0)
			{
				LPRE[N - 1] = 0;
				LACT[N - 1] = 0;
			}
			unsigned int tempLatency = 0;
			if(impl == "B") {				
				if (req_statues_flag[requestor] == true)
				{
					if (req_open[requestor] == true)
					{
						if (reqNotEmpty(requestor))
						{
							if (returnOldest(requestor)->busPacketType == RD)
							{
								if (isReadyTimer(returnOldest(requestor), requestor) <= 1)
								{
									if (CAStimer == 0)
										temp_timer[requestor] = 1 + front_has_rd * tCCD + tRL + tBUS;
									else
										temp_timer[requestor] = CAStimer + front_has_rd * tCCD + tRL + tBUS;
								}
								else
								{
									temp_timer[requestor] = isReadyTimer(returnOldest(requestor), requestor) + (front_has_rd - 2) * tCCD + max(tRTW, 2 * tCCD) + tWtoR - 1 + tRL + tBUS;
								}
								tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor]:tempLatency; 
								temp_timer[requestor] = 0;
								
								if (isReadyTimer(returnOldest(requestor), requestor) <= 1)
								{
									if (CAStimer == 0)
										temp_timer[requestor] = 1 + front_has_rd * tCCD + tRL + tBUS;
									else
										temp_timer[requestor] = CAStimer + front_has_rd * tCCD + tRL + tBUS;
								}
								else
								{
									temp_timer[requestor] = isReadyTimer(returnOldest(requestor), requestor) + (front_has_rd - 2) * tCCD + max(tRTW, 2 * tCCD) + tWtoR - 1 + tRL + tBUS;
								}
								tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor]:tempLatency; 
								temp_timer[requestor] = 0;
								
								if (isReadyTimer(returnOldest(requestor), requestor) <= 1)
								{
									if (CAStimer == 0)
										temp_timer[requestor] = 1 + front_has_rd * tCCD + tRL + tBUS;
									else
										temp_timer[requestor] = CAStimer + front_has_rd * tCCD + tRL + tBUS;
								}
								else
								{
									temp_timer[requestor] = isReadyTimer(returnOldest(requestor), requestor) + (front_has_rd - 2) * tCCD + max(tRTW, 2 * tCCD) + tWtoR - 1 + tRL + tBUS;
								}
								
								tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor]:tempLatency; 
								temp_timer[requestor] = 0;
							
								if (isReadyTimer(returnOldest(requestor), requestor) <= 1)
								{
									temp_timer[requestor] = tCCD + front_has_rd * tCCD + tRL + tBUS;
								}
								else
								{
									temp_timer[requestor] = isReadyTimer(returnOldest(requestor), requestor) + (front_has_rd - 2) * tCCD + max(tRTW, 2 * tCCD) + tWtoR - 1 + tRL + tBUS;
								}
								tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor]:tempLatency; 
								temp_timer[requestor] = 0;	
							
								if (isReadyTimer(returnOldest(requestor), requestor) <= 1)
								{
									temp_timer[requestor] = tWtoR + front_has_rd * tCCD + tRL + tBUS;
								}
								else
								{
									temp_timer[requestor] = isReadyTimer(returnOldest(requestor), requestor) + (front_has_rd - 2) * tCCD + max(tRTW, 2 * tCCD) + tWtoR - 1 + tRL + tBUS;
								}
								tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor]:tempLatency; 
								temp_timer[requestor] = 0;
																	
								temp_timer[requestor] = tRL + tBUS;
								tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor]:tempLatency; 
								temp_timer[requestor] = 0;
							
							}
							else if (returnOldest(requestor)->busPacketType == WR)
							{
								if (isReadyTimer(returnOldest(requestor), requestor) <= 1)
								{
									if (CAStimer == 0)
										temp_timer[requestor] = 1 + (REQ_count - 1) * tCCD + tRTW + tWL + tBUS;
									else
										temp_timer[requestor] = CAStimer + (REQ_count - 1) * tCCD + tRTW + tWL + tBUS;
								}
								else
								{
									temp_timer[requestor] = isReadyTimer(returnOldest(requestor), requestor) + (REQ_count - 1) * tCCD + tRTW + 1 + tWL + tBUS;
								}
								tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor]:tempLatency; 
								temp_timer[requestor] = 0;
							
								if (isReadyTimer(returnOldest(requestor), requestor) <= 1)
								{
									if (CAStimer == 0)
										temp_timer[requestor] = 1 + (REQ_count - 1) * tCCD + tRTW + tWL + tBUS;
									else
										temp_timer[requestor] = CAStimer + (REQ_count - 1) * tCCD + tRTW + tWL + tBUS;
								}
								else
								{
									temp_timer[requestor] = isReadyTimer(returnOldest(requestor), requestor) + (REQ_count - 1) * tCCD + tRTW + 1 + tWL + tBUS;
								}
								tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor]:tempLatency; 
								temp_timer[requestor] = 0;
							
								if (isReadyTimer(returnOldest(requestor), requestor) <= 1)
								{
									if (CAStimer == 0)
										temp_timer[requestor] = 1 + (REQ_count - 1) * tCCD + tRTW + tWL + tBUS;
									else
										temp_timer[requestor] = CAStimer + (REQ_count - 1) * tCCD + tRTW + tWL + tBUS;
								}
								else
								{
									temp_timer[requestor] = isReadyTimer(returnOldest(requestor), requestor) + (REQ_count - 1) * tCCD + tRTW + 1 + tWL + tBUS;
								}
								tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor]:tempLatency; 
								temp_timer[requestor] = 0;
							
								if (isReadyTimer(returnOldest(requestor), requestor) <= 1)
								{
									temp_timer[requestor] = tRTW + (REQ_count - 1) * tCCD + tRTW + tWL + tBUS;
								}
								else
								{
									temp_timer[requestor] = isReadyTimer(returnOldest(requestor), requestor) + (REQ_count - 1) * tCCD + tRTW + 1 + tWL + tBUS;
								}
								tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor]:tempLatency; 
								temp_timer[requestor] = 0;
							
								if (isReadyTimer(returnOldest(requestor), requestor) <= 1)
								{
									temp_timer[requestor] = tCCD + (REQ_count - 1) * tCCD + tRTW + tWL + tBUS;
								}
								else
								{
									temp_timer[requestor] = isReadyTimer(returnOldest(requestor), requestor) + (REQ_count - 1) * tCCD + tRTW + 1 + tWL + tBUS;
								}
								tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor]:tempLatency; 
								temp_timer[requestor] = 0;
						
								temp_timer[requestor] = tWL + tBUS;
								tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor]:tempLatency; 
								temp_timer[requestor] = 0;
							
							}
						}
					}
					else if (req_open[requestor] == false)
					{
						if (returnOldest(requestor)->busPacketType == PRE)
						{
							req_open[requestor] = false;
						
							temp_timer[requestor] = max(isReadyTimer(returnOldest(requestor), requestor), tRTP) +
															LPRE[front_has_pre] + tRP + LACT[front_has_act] + tRCD + (front_has_rd - 2) * tCCD + max(tRTW, 2 * tCCD) + tWtoR - 1 + tRL + tBUS;
							tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor]:tempLatency; 
							temp_timer[requestor] = 0;
					
									
							temp_timer[requestor] = max(isReadyTimer(returnOldest(requestor), requestor), tWR) +
															LPRE[front_has_pre] + tRP + LACT[front_has_act] + tRCD + (front_has_rd - 2) * tCCD + max(tRTW, 2 * tCCD) + tWtoR - 1 + tRL + tBUS;
							tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor]:tempLatency; 
							temp_timer[requestor] = 0;							
						
							temp_timer[requestor] = tRP + LACT[front_has_act] + tRCD + (front_has_rd - 2) * tCCD + max(tRTW, 2 * tCCD) + tWtoR - 1 + tRL + tBUS;
							tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor]:tempLatency; 
							temp_timer[requestor] = 0;
						
							temp_timer[requestor] = isReadyTimer(returnOldest(requestor), requestor) + LPRE[front_has_pre] + tRP + LACT[front_has_act] + tRCD + (front_has_rd - 2) * tCCD + max(tRTW, 2 * tCCD) + tWtoR - 1 + tRL + tBUS;
							tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor]:tempLatency; 
							temp_timer[requestor] = 0;
						
								
							if (isReadyTimer(returnOldest(requestor), requestor) > 0)
								temp_timer[requestor] = isReadyTimer(returnOldest(requestor), requestor) + LPRE[front_has_pre] + tRP + LACT[front_has_act] + tRCD + (front_has_rd - 2) * tCCD + max(tRTW, 2 * tCCD) + tWtoR - 1 + tRL + tBUS;
							else if (isReadyTimer(returnOldest(requestor), requestor) == 0)
								temp_timer[requestor] = isReadyTimer(returnOldest(requestor), requestor) + LPRE[front_has_pre] + tRP + LACT[front_has_act] + tRCD + (front_has_rd - 2) * tCCD + max(tRTW, 2 * tCCD) + tWtoR - 1 + 1 + tRL + tBUS;
							tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor]:tempLatency; 
							temp_timer[requestor] = 0;								
						
						}
						else if (returnOldest(requestor)->busPacketType == ACT)
						{
							
							temp_timer[requestor] = isReadyTimer(returnOldest(requestor), requestor) + LACT[front_has_act] + tRCD + (front_has_rd - 2) * tCCD + max(tRTW, 2 * tCCD) + tWtoR - 1 + tRL + tBUS;
							tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor]:tempLatency; 
							temp_timer[requestor] = 0;
						
							temp_timer[requestor] = tRCD + (front_has_rd - 2) * tCCD + max(tRTW, 2 * tCCD) + tWtoR - 1 + tRL + tBUS;
							tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor]:tempLatency; 
							temp_timer[requestor] = 0;	
						
							temp_timer[requestor] = isReadyTimer(returnOldest(requestor), requestor) + LACT[front_has_act] + tRCD + (front_has_rd - 2) * tCCD + max(tRTW, 2 * tCCD) + tWtoR - 1 + tRL + tBUS;
							tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor]:tempLatency; 
							temp_timer[requestor] = 0;							
							
						}
						else if (returnOldest(requestor)->busPacketType == RD)
						{
						
							if (isReadyTimer(returnOldest(requestor), requestor) <= 1)
							{
								if (CAStimer == 0)
									temp_timer[requestor] = 1 + front_has_rd * tCCD + tRL + tBUS;
								else
									temp_timer[requestor] = CAStimer + front_has_rd * tCCD + tRL + tBUS;

								tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor]:tempLatency; 
								temp_timer[requestor] = 0;	
							}
							else
							{
								temp_timer[requestor] = isReadyTimer(returnOldest(requestor), requestor) + (front_has_rd - 2) * tCCD + max(tRTW, 2 * tCCD) + tWtoR - 1 + tRL + tBUS;
								tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor]:tempLatency; 
								temp_timer[requestor] = 0;
							}
							if (isReadyTimer(returnOldest(requestor), requestor) <= 1)
							{
								temp_timer[requestor] = tCCD + front_has_rd * tCCD + tRL + tBUS;
								tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor]:tempLatency; 
								temp_timer[requestor] = 0;
							}
							else
							{
								temp_timer[requestor] = isReadyTimer(returnOldest(requestor), requestor) + (front_has_rd - 2) * tCCD + max(tRTW, 2 * tCCD) + tWtoR - 1 + tRL + tBUS;
								tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor]:tempLatency; 
								temp_timer[requestor] = 0;
							}
							if (isReadyTimer(returnOldest(requestor), requestor) <= 1)
							{
								temp_timer[requestor] = tWtoR + front_has_rd * tCCD + tRL + tBUS;
								tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor]:tempLatency; 
								temp_timer[requestor] = 0;
							}
							else
							{
								temp_timer[requestor] = isReadyTimer(returnOldest(requestor), requestor) + (front_has_rd - 2) * tCCD + max(tRTW, 2 * tCCD) + tWtoR - 1 + tRL + tBUS;
								tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor]:tempLatency; 
								temp_timer[requestor] = 0;
							}
							temp_timer[requestor] = tRL + tBUS;
							tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor]:tempLatency; 
							temp_timer[requestor] = 0;
						
						}
						else if (returnOldest(requestor)->busPacketType == WR)
						{
						
							if (isReadyTimer(returnOldest(requestor), requestor) <= 1)
							{
								if (CAStimer == 0)
									temp_timer[requestor] = 1 + (REQ_count - 1) * tCCD + tRTW + tWL + tBUS;
								else
									temp_timer[requestor] = CAStimer + (REQ_count - 1) * tCCD + tRTW + tWL + tBUS;
								
								tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor]:tempLatency; 
								temp_timer[requestor] = 0;	
							}
							else
							{
								temp_timer[requestor] = isReadyTimer(returnOldest(requestor), requestor) + (REQ_count - 1) * tCCD + tRTW + 1 + tWL + tBUS;
								tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor]:tempLatency; 
								temp_timer[requestor] = 0;
							}
							if (isReadyTimer(returnOldest(requestor), requestor) <= 1)
							{
								if (CAStimer == 0)
									temp_timer[requestor] = 1 + (REQ_count - 1) * tCCD + tRTW + tWL + tBUS;
								else
									temp_timer[requestor] = CAStimer + (REQ_count - 1) * tCCD + tRTW + tWL + tBUS;

								tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor]:tempLatency; 
								temp_timer[requestor] = 0;	
							}
							else
							{
								temp_timer[requestor] = isReadyTimer(returnOldest(requestor), requestor) + (REQ_count - 1) * tCCD + tRTW + 1 + tWL + tBUS;
								tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor]:tempLatency; 
								temp_timer[requestor] = 0;	
							}
							if (isReadyTimer(returnOldest(requestor), requestor) <= 1)
							{
								if (CAStimer == 0)
									temp_timer[requestor] = 1 + (REQ_count - 1) * tCCD + tRTW + tWL + tBUS;
								else
									temp_timer[requestor] = CAStimer + (REQ_count - 1) * tCCD + tRTW + tWL + tBUS;

								tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor]:tempLatency; 
								temp_timer[requestor] = 0;		
							}
							else
							{
								temp_timer[requestor] = isReadyTimer(returnOldest(requestor), requestor) + (REQ_count - 1) * tCCD + tRTW + 1 + tWL + tBUS;
								tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor]:tempLatency; 
								temp_timer[requestor] = 0;	
							}
							if (isReadyTimer(returnOldest(requestor), requestor) <= 1)
							{
								temp_timer[requestor] = tRTW + front_has_rd * tCCD + (REQ_count - 1) * tCCD + tRTW + tWL + tBUS;
								tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor]:tempLatency; 
								temp_timer[requestor] = 0;	
							}
							else
							{
								temp_timer[requestor] = isReadyTimer(returnOldest(requestor), requestor) + (REQ_count - 1) * tCCD + tRTW + 1 + tWL + tBUS;
								tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor]:tempLatency; 
								temp_timer[requestor] = 0;	
							}
							if (isReadyTimer(returnOldest(requestor), requestor) <= 1)
							{
								temp_timer[requestor] = tCCD + (REQ_count - 1) * tCCD + tRTW + tWL + tBUS;
								tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor]:tempLatency; 
								temp_timer[requestor] = 0;	
							}
							else
							{
								temp_timer[requestor] = isReadyTimer(returnOldest(requestor), requestor) + (REQ_count - 1) * tCCD + tRTW + 1 + tWL + tBUS;
								tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor]:tempLatency; 
								temp_timer[requestor] = 0;	
							}
							temp_timer[requestor] = tWL + tBUS;
							tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor]:tempLatency; 
							temp_timer[requestor] = 0;	
							
						}
					}
				}
				else if (req_statues_flag[requestor] == false)
				{
					if (reqNotEmpty(requestor))
					{
						req_statues_flag[requestor] = true;
						if (returnOldest(requestor)->busPacketType == RD)
						{
							req_open[requestor] = true;
							
							if (isReadyTimer(returnOldest(requestor), requestor) <= 1)
							{
								if (CAStimer == 0)
									temp_timer[requestor] = 1 + front_has_rd * tCCD + tRL + tBUS;
								else
									temp_timer[requestor] = CAStimer + front_has_rd * tCCD + tRL + tBUS;

								tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor]:tempLatency; 
								temp_timer[requestor] = 0;		
							}
							else
							{
								temp_timer[requestor] = isReadyTimer(returnOldest(requestor), requestor) + (front_has_rd - 2) * tCCD + max(tRTW, 2 * tCCD) + tWtoR - 1 + tRL + tBUS;
								tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor]:tempLatency; 
								temp_timer[requestor] = 0;	
							}
							
							if (isReadyTimer(returnOldest(requestor), requestor) <= 1)
							{
								temp_timer[requestor] = tCCD + front_has_rd * tCCD + tRL + tBUS;
								tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor]:tempLatency; 
								temp_timer[requestor] = 0;	
							}
							else
							{
								temp_timer[requestor] = isReadyTimer(returnOldest(requestor), requestor) + (front_has_rd - 2) * tCCD + max(tRTW, 2 * tCCD) + tWtoR - 1 + tRL + tBUS;
								tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor]:tempLatency; 
								temp_timer[requestor] = 0;	
							}
							if (isReadyTimer(returnOldest(requestor), requestor) <= 1)
							{
								temp_timer[requestor] = tWtoR + front_has_rd * tCCD + tRL + tBUS;
								tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor]:tempLatency; 
								temp_timer[requestor] = 0;	
							}
							else
							{
								temp_timer[requestor] = isReadyTimer(returnOldest(requestor), requestor) + (front_has_rd - 2) * tCCD + max(tRTW, 2 * tCCD) + tWtoR - 1 + tRL + tBUS;
								tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor]:tempLatency; 
								temp_timer[requestor] = 0;	
							}
						
								temp_timer[requestor] = tRL + tBUS;
								tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor]:tempLatency; 
								temp_timer[requestor] = 0;	
						
						}
						else if (returnOldest(requestor)->busPacketType == WR)
						{
							req_open[requestor] = true;
							// if (cmd == NULL)
							// {
							if (isReadyTimer(returnOldest(requestor), requestor) <= 1)
							{
								if (CAStimer == 0)
									temp_timer[requestor] = 1 + (REQ_count - 1) * tCCD + tRTW + tWL + tBUS;
								else
									temp_timer[requestor] = CAStimer + (REQ_count - 1) * tCCD + tRTW + tWL + tBUS;

								tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor]:tempLatency; 
								temp_timer[requestor] = 0;		
							}
							else
							{
								temp_timer[requestor] = isReadyTimer(returnOldest(requestor), requestor) + (REQ_count - 1) * tCCD + tRTW + 1 + tWL + tBUS;
								tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor]:tempLatency; 
								temp_timer[requestor] = 0;	
							}
							// }
							// else if (cmd->busPacketType == PRE && cmd->requestorID != requestor)
							// {
								// if (isReadyTimer(returnOldest(requestor), requestor) <= 1)
								// {
								// 	if (CAStimer == 0)
								// 		temp_timer[requestor] = 1 + (REQ_count - 1) * tCCD + tRTW + tWL + tBUS;
								// 	else
								// 		temp_timer[requestor] = CAStimer + (REQ_count - 1) * tCCD + tRTW + tWL + tBUS;
								// }
								// else
								// {
								// 	temp_timer[requestor] = isReadyTimer(returnOldest(requestor), requestor) + (REQ_count - 1) * tCCD + tRTW + 1 + tWL + tBUS;
								// }
							// }
							// else if (cmd->busPacketType == ACT && cmd->requestorID != requestor)
							// {
							// 	if (isReadyTimer(returnOldest(requestor), requestor) <= 1)
							// 	{
							// 		if (CAStimer == 0)
							// 			temp_timer[requestor] = 1 + (REQ_count - 1) * tCCD + tRTW + tWL + tBUS;
							// 		else
							// 			temp_timer[requestor] = CAStimer + (REQ_count - 1) * tCCD + tRTW + tWL + tBUS;
							// 	}
							// 	else
							// 	{
							// 		temp_timer[requestor] = isReadyTimer(returnOldest(requestor), requestor) + (REQ_count - 1) * tCCD + tRTW + 1 + tWL + tBUS;
							// 	}
							// }
							// else if (cmd->busPacketType == RD && cmd->requestorID != requestor)
							// {
							if (isReadyTimer(returnOldest(requestor), requestor) <= 1)
							{
								temp_timer[requestor] = tRTW + (REQ_count - 1) * tCCD + tRTW + tWL + tBUS;
								tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor]:tempLatency; 
								temp_timer[requestor] = 0;	
							}
							else
							{
								temp_timer[requestor] = isReadyTimer(returnOldest(requestor), requestor) + (REQ_count - 1) * tCCD + tRTW + 1 + tWL + tBUS;
								tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor]:tempLatency; 
								temp_timer[requestor] = 0;	
							}
							// }
							// else if (cmd->busPacketType == WR && cmd->requestorID != requestor)
							// {
							if (isReadyTimer(returnOldest(requestor), requestor) <= 1)
							{
								temp_timer[requestor] = tCCD + (REQ_count - 1) * tCCD + tRTW + tWL + tBUS;
								tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor]:tempLatency; 
								temp_timer[requestor] = 0;	
							}
							else
							{
								temp_timer[requestor] = isReadyTimer(returnOldest(requestor), requestor) + (REQ_count - 1) * tCCD + tRTW + 1 + tWL + tBUS;
								tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor]:tempLatency; 
								temp_timer[requestor] = 0;	
							}
							// }
							// else if (cmd->busPacketType == WR && cmd->requestorID == requestor)
							// {
							temp_timer[requestor] = tWL + tBUS;
							tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor]:tempLatency; 
							temp_timer[requestor] = 0;	
							// }
						}
						else if (returnOldest(requestor)->busPacketType == PRE) // done modifications
						{
							req_open[requestor] = false;
							
							temp_timer[requestor] = max(isReadyTimer(returnOldest(requestor), requestor), tRTP) +
																LPRE[front_has_pre] + tRP + LACT[front_has_act] + tRCD + (front_has_rd - 2) * tCCD + max(tRTW, 2 * tCCD) + tWtoR - 1 + tRL + tBUS;
							tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor]:tempLatency; 
							temp_timer[requestor] = 0;																
								
							temp_timer[requestor] = max(isReadyTimer(returnOldest(requestor), requestor), tWR) + LPRE[front_has_pre] + tRP + LACT[front_has_act] + tRCD + (front_has_rd - 2) * tCCD + max(tRTW, 2 * tCCD) + tWtoR - 1 + tRL + tBUS;
							tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor]:tempLatency; 
							temp_timer[requestor] = 0;	
								
							temp_timer[requestor] = tRP + LACT[front_has_act] + tRCD + (front_has_rd - 2) * tCCD + max(tRTW, 2 * tCCD) + tWtoR - 1 + tRL + tBUS;
							tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor]:tempLatency; 
							temp_timer[requestor] = 0;	
							
							temp_timer[requestor] = isReadyTimer(returnOldest(requestor), requestor) + LPRE[front_has_pre] + tRP + LACT[front_has_act] + tRCD + (front_has_rd - 2) * tCCD + max(tRTW, 2 * tCCD) + tWtoR - 1 + tRL + tBUS;
							tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor]:tempLatency; 
							temp_timer[requestor] = 0;	
							
							if (isReadyTimer(returnOldest(requestor), requestor) > 0)
								temp_timer[requestor] = isReadyTimer(returnOldest(requestor), requestor) + LPRE[front_has_pre] + tRP + LACT[front_has_act] + tRCD + (front_has_rd - 2) * tCCD + max(tRTW, 2 * tCCD) + tWtoR - 1 + tRL + tBUS;
							else if (isReadyTimer(returnOldest(requestor), requestor) == 0)
								temp_timer[requestor] = isReadyTimer(returnOldest(requestor), requestor) + LPRE[front_has_pre] + tRP + LACT[front_has_act] + tRCD + (front_has_rd - 2) * tCCD + max(tRTW, 2 * tCCD) + tWtoR - 1 + 1 + tRL + tBUS;
							
							tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor]:tempLatency; 
							temp_timer[requestor] = 0;
							
						}
					}
					else
					{
						temp_timer[requestor] = 0;
						tempLatency = temp_timer[requestor];
					}
				}
			}
			else if(impl == "C") 
			{
				if (req_statues_flag[requestor] == true)
				{

					if (req_open[requestor] == true)
					{

						if (reqNotEmpty(requestor))
						{

							if (returnOldest(requestor)->busPacketType == RD)
							{
								if (cmd == NULL)
								{
									if (isReadyTimer(returnOldest(requestor), requestor) <= 1)
									{
										if (CAStimer == 0)
											temp_timer[requestor] = 1 + front_has_rd * tCCD + tRL + tBUS;
										else
											temp_timer[requestor] = CAStimer + front_has_rd * tCCD + tRL + tBUS;
									}
									else
									{
										temp_timer[requestor] = isReadyTimer(returnOldest(requestor), requestor) + (front_has_rd - 2) * tCCD + max(tRTW, 2 * tCCD) + tWtoR - 1 + tRL + tBUS;
									}
								}
								else if (cmd->busPacketType == PRE && cmd->requestorID != requestor)
								{
									if (isReadyTimer(returnOldest(requestor), requestor) <= 1)
									{
										if (CAStimer == 0)
											temp_timer[requestor] = 1 + front_has_rd * tCCD + tRL + tBUS;
										else
											temp_timer[requestor] = CAStimer + front_has_rd * tCCD + tRL + tBUS;
									}
									else
									{
										temp_timer[requestor] = isReadyTimer(returnOldest(requestor), requestor) + (front_has_rd - 2) * tCCD + max(tRTW, 2 * tCCD) + tWtoR - 1 + tRL + tBUS;
									}
								}
								else if (cmd->busPacketType == ACT && cmd->requestorID != requestor)
								{
									if (isReadyTimer(returnOldest(requestor), requestor) <= 1)
									{
										if (CAStimer == 0)
											temp_timer[requestor] = 1 + front_has_rd * tCCD + tRL + tBUS;
										else
											temp_timer[requestor] = CAStimer + front_has_rd * tCCD + tRL + tBUS;
									}
									else
									{
										temp_timer[requestor] = isReadyTimer(returnOldest(requestor), requestor) + (front_has_rd - 2) * tCCD + max(tRTW, 2 * tCCD) + tWtoR - 1 + tRL + tBUS;
									}
								}
								else if (cmd->busPacketType == RD && cmd->requestorID != requestor)
								{
									if (isReadyTimer(returnOldest(requestor), requestor) <= 1)
									{
										temp_timer[requestor] = tCCD + front_has_rd * tCCD + tRL + tBUS;
									}
									else
									{
										temp_timer[requestor] = isReadyTimer(returnOldest(requestor), requestor) + (front_has_rd - 2) * tCCD + max(tRTW, 2 * tCCD) + tWtoR - 1 + tRL + tBUS;
									}
								}
								else if (cmd->busPacketType == WR && cmd->requestorID != requestor)
								{
									if (isReadyTimer(returnOldest(requestor), requestor) <= 1)
									{
										temp_timer[requestor] = tWtoR + front_has_rd * tCCD + tRL + tBUS;
									}
									else
									{
										temp_timer[requestor] = isReadyTimer(returnOldest(requestor), requestor) + (front_has_rd - 2) * tCCD + max(tRTW, 2 * tCCD) + tWtoR - 1 + tRL + tBUS;
									}
								}
								else if (cmd->busPacketType == RD && cmd->requestorID == requestor)
								{
									temp_timer[requestor] = tRL + tBUS;
								}
							}
							else if (returnOldest(requestor)->busPacketType == WR)
							{
								if (cmd == NULL)
								{
									if (isReadyTimer(returnOldest(requestor), requestor) <= 1)
									{
										if (CAStimer == 0)
											temp_timer[requestor] = 1 + (REQ_count - 1) * tCCD + tRTW + tWL + tBUS;
										else
											temp_timer[requestor] = CAStimer + (REQ_count - 1) * tCCD + tRTW + tWL + tBUS;
									}
									else
									{
										temp_timer[requestor] = isReadyTimer(returnOldest(requestor), requestor) + (REQ_count - 1) * tCCD + tRTW + 1 + tWL + tBUS;
									}
								}
								else if (cmd->busPacketType == PRE && cmd->requestorID != requestor)
								{
									if (isReadyTimer(returnOldest(requestor), requestor) <= 1)
									{
										if (CAStimer == 0)
											temp_timer[requestor] = 1 + (REQ_count - 1) * tCCD + tRTW + tWL + tBUS;
										else
											temp_timer[requestor] = CAStimer + (REQ_count - 1) * tCCD + tRTW + tWL + tBUS;
									}
									else
									{
										temp_timer[requestor] = isReadyTimer(returnOldest(requestor), requestor) + (REQ_count - 1) * tCCD + tRTW + 1 + tWL + tBUS;
									}
								}
								else if (cmd->busPacketType == ACT && cmd->requestorID != requestor)
								{
									if (isReadyTimer(returnOldest(requestor), requestor) <= 1)
									{
										if (CAStimer == 0)
											temp_timer[requestor] = 1 + (REQ_count - 1) * tCCD + tRTW + tWL + tBUS;
										else
											temp_timer[requestor] = CAStimer + (REQ_count - 1) * tCCD + tRTW + tWL + tBUS;
									}
									else
									{
										temp_timer[requestor] = isReadyTimer(returnOldest(requestor), requestor) + (REQ_count - 1) * tCCD + tRTW + 1 + tWL + tBUS;
									}
								}
								else if (cmd->busPacketType == RD && cmd->requestorID != requestor)
								{
									if (isReadyTimer(returnOldest(requestor), requestor) <= 1)
									{
										temp_timer[requestor] = tRTW + (REQ_count - 1) * tCCD + tRTW + tWL + tBUS;
									}
									else
									{
										temp_timer[requestor] = isReadyTimer(returnOldest(requestor), requestor) + (REQ_count - 1) * tCCD + tRTW + 1 + tWL + tBUS;
									}
								}
								else if (cmd->busPacketType == WR && cmd->requestorID != requestor)
								{
									if (isReadyTimer(returnOldest(requestor), requestor) <= 1)
									{
										temp_timer[requestor] = tCCD + (REQ_count - 1) * tCCD + tRTW + tWL + tBUS;
									}
									else
									{
										temp_timer[requestor] = isReadyTimer(returnOldest(requestor), requestor) + (REQ_count - 1) * tCCD + tRTW + 1 + tWL + tBUS;
									}
								}
								else if (cmd->busPacketType == WR && cmd->requestorID == requestor)
								{
									temp_timer[requestor] = tWL + tBUS;
								}
							}
						}
					}
					else if (req_open[requestor] == false)
					{
						if (returnOldest(requestor)->busPacketType == PRE)
						{
							if (cmd != NULL)
							{
								if (cmd->requestorID == requestor)
								{
									if (cmd->busPacketType == RD)
									{
										req_open[requestor] = false;
										temp_timer[requestor] = max(isReadyTimer(returnOldest(requestor), requestor), tRTP) +
																LPRE[front_has_pre] + tRP + LACT[front_has_act] + tRCD + (front_has_rd - 2) * tCCD + max(tRTW, 2 * tCCD) + tWtoR - 1 + tRL + tBUS;
									}
									if (cmd->busPacketType == WR)
									{
										req_open[requestor] = false;
										temp_timer[requestor] = max(isReadyTimer(returnOldest(requestor), requestor), tWR) +
																LPRE[front_has_pre] + tRP + LACT[front_has_act] + tRCD + (front_has_rd - 2) * tCCD + max(tRTW, 2 * tCCD) + tWtoR - 1 + tRL + tBUS;
									}
									else
									{
										req_open[requestor] = false;
										temp_timer[requestor] = tRP + LACT[front_has_act] + tRCD + (front_has_rd - 2) * tCCD + max(tRTW, 2 * tCCD) + tWtoR - 1 + tRL + tBUS;
									}
								}
								else
								{
									req_open[requestor] = false;
									temp_timer[requestor] = isReadyTimer(returnOldest(requestor), requestor) + LPRE[front_has_pre] + tRP + LACT[front_has_act] + tRCD + (front_has_rd - 2) * tCCD + max(tRTW, 2 * tCCD) + tWtoR - 1 + tRL + tBUS;
								}
							}
							else
							{
								req_open[requestor] = false;
								if (isReadyTimer(returnOldest(requestor), requestor) > 0)
									temp_timer[requestor] = isReadyTimer(returnOldest(requestor), requestor) + LPRE[front_has_pre] + tRP + LACT[front_has_act] + tRCD + (front_has_rd - 2) * tCCD + max(tRTW, 2 * tCCD) + tWtoR - 1 + tRL + tBUS;
								else if (isReadyTimer(returnOldest(requestor), requestor) == 0)
									temp_timer[requestor] = isReadyTimer(returnOldest(requestor), requestor) + LPRE[front_has_pre] + tRP + LACT[front_has_act] + tRCD + (front_has_rd - 2) * tCCD + max(tRTW, 2 * tCCD) + tWtoR - 1 + 1 + tRL + tBUS;
							}
						}
						else if (returnOldest(requestor)->busPacketType == ACT)
						{
							if (cmd != NULL)
							{
								if (requestor != cmd->requestorID)
								{
									temp_timer[requestor] = isReadyTimer(returnOldest(requestor), requestor) + LACT[front_has_act] + tRCD + (front_has_rd - 2) * tCCD + max(tRTW, 2 * tCCD) + tWtoR - 1 + tRL + tBUS;
								}
								else
								{
									temp_timer[requestor] = tRCD + (front_has_rd - 2) * tCCD + max(tRTW, 2 * tCCD) + tWtoR - 1 + tRL + tBUS;
								}
							}
							else
							{
								temp_timer[requestor] = isReadyTimer(returnOldest(requestor), requestor) + LACT[front_has_act] + tRCD + (front_has_rd - 2) * tCCD + max(tRTW, 2 * tCCD) + tWtoR - 1 + tRL + tBUS;
							}
						}
						else if (returnOldest(requestor)->busPacketType == RD)
						{
							if (cmd == NULL)
							{
								if (isReadyTimer(returnOldest(requestor), requestor) <= 1)
								{
									if (CAStimer == 0)
										temp_timer[requestor] = 1 + front_has_rd * tCCD + tRL + tBUS;
									else
										temp_timer[requestor] = CAStimer + front_has_rd * tCCD + tRL + tBUS;
								}
								else
								{
									temp_timer[requestor] = isReadyTimer(returnOldest(requestor), requestor) + (front_has_rd - 2) * tCCD + max(tRTW, 2 * tCCD) + tWtoR - 1 + tRL + tBUS;
								}
							}
							else if (cmd->busPacketType == PRE && cmd->requestorID != requestor)
							{
								if (isReadyTimer(returnOldest(requestor), requestor) <= 1)
								{
									if (CAStimer == 0)
										temp_timer[requestor] = 1 + front_has_rd * tCCD + tRL + tBUS;
									else
										temp_timer[requestor] = CAStimer + front_has_rd * tCCD + tRL + tBUS;
									;
								}
								else
								{
									temp_timer[requestor] = isReadyTimer(returnOldest(requestor), requestor) + (front_has_rd - 2) * tCCD + max(tRTW, 2 * tCCD) + tWtoR - 1 + tRL + tBUS;
								}
							}
							else if (cmd->busPacketType == ACT && cmd->requestorID != requestor)
							{
								if (isReadyTimer(returnOldest(requestor), requestor) <= 1)
								{
									if (CAStimer == 0)
										temp_timer[requestor] = 1 + front_has_rd * tCCD + tRL + tBUS;
									else
										temp_timer[requestor] = CAStimer + front_has_rd * tCCD + tRL + tBUS;
								}
								else
								{
									temp_timer[requestor] = isReadyTimer(returnOldest(requestor), requestor) + (front_has_rd - 2) * tCCD + max(tRTW, 2 * tCCD) + tWtoR - 1 + tRL + tBUS;
								}
							}
							else if (cmd->busPacketType == RD && cmd->requestorID != requestor)
							{
								if (isReadyTimer(returnOldest(requestor), requestor) <= 1)
								{
									temp_timer[requestor] = tCCD + front_has_rd * tCCD + tRL + tBUS;
								}
								else
								{
									temp_timer[requestor] = isReadyTimer(returnOldest(requestor), requestor) + (front_has_rd - 2) * tCCD + max(tRTW, 2 * tCCD) + tWtoR - 1 + tRL + tBUS;
								}
							}
							else if (cmd->busPacketType == WR && cmd->requestorID != requestor)
							{
								if (isReadyTimer(returnOldest(requestor), requestor) <= 1)
								{
									temp_timer[requestor] = tWtoR + front_has_rd * tCCD + tRL + tBUS;
								}
								else
								{
									temp_timer[requestor] = isReadyTimer(returnOldest(requestor), requestor) + (front_has_rd - 2) * tCCD + max(tRTW, 2 * tCCD) + tWtoR - 1 + tRL + tBUS;
								}
							}
							else if (cmd->busPacketType == RD && cmd->requestorID == requestor)
							{
								temp_timer[requestor] = tRL + tBUS;
							}
						}

						else if (returnOldest(requestor)->busPacketType == WR)
						{
							if (cmd == NULL)
							{
								if (isReadyTimer(returnOldest(requestor), requestor) <= 1)
								{
									if (CAStimer == 0)
										temp_timer[requestor] = 1 + (REQ_count - 1) * tCCD + tRTW + tWL + tBUS;
									else
										temp_timer[requestor] = CAStimer + (REQ_count - 1) * tCCD + tRTW + tWL + tBUS;
								}
								else
								{
									temp_timer[requestor] = isReadyTimer(returnOldest(requestor), requestor) + (REQ_count - 1) * tCCD + tRTW + 1 + tWL + tBUS;
								}
							}
							else if (cmd->busPacketType == PRE && cmd->requestorID != requestor)
							{
								if (isReadyTimer(returnOldest(requestor), requestor) <= 1)
								{
									if (CAStimer == 0)
										temp_timer[requestor] = 1 + (REQ_count - 1) * tCCD + tRTW + tWL + tBUS;
									else
										temp_timer[requestor] = CAStimer + (REQ_count - 1) * tCCD + tRTW + tWL + tBUS;
								}
								else
								{
									temp_timer[requestor] = isReadyTimer(returnOldest(requestor), requestor) + (REQ_count - 1) * tCCD + tRTW + 1 + tWL + tBUS;
								}
							}
							else if (cmd->busPacketType == ACT && cmd->requestorID != requestor)
							{
								if (isReadyTimer(returnOldest(requestor), requestor) <= 1)
								{
									if (CAStimer == 0)
										temp_timer[requestor] = 1 + (REQ_count - 1) * tCCD + tRTW + tWL + tBUS;
									else
										temp_timer[requestor] = CAStimer + (REQ_count - 1) * tCCD + tRTW + tWL + tBUS;
								}
								else
								{
									temp_timer[requestor] = isReadyTimer(returnOldest(requestor), requestor) + (REQ_count - 1) * tCCD + tRTW + 1 + tWL + tBUS;
								}
							}
							else if (cmd->busPacketType == RD && cmd->requestorID != requestor)
							{
								if (isReadyTimer(returnOldest(requestor), requestor) <= 1)
								{
									temp_timer[requestor] = tRTW + front_has_rd * tCCD + (REQ_count - 1) * tCCD + tRTW + tWL + tBUS;
								}
								else
								{
									temp_timer[requestor] = isReadyTimer(returnOldest(requestor), requestor) + (REQ_count - 1) * tCCD + tRTW + 1 + tWL + tBUS;
								}
							}
							else if (cmd->busPacketType == WR && cmd->requestorID != requestor)
							{
								if (isReadyTimer(returnOldest(requestor), requestor) <= 1)
								{
									temp_timer[requestor] = tCCD + (REQ_count - 1) * tCCD + tRTW + tWL + tBUS;
								}
								else
								{
									temp_timer[requestor] = isReadyTimer(returnOldest(requestor), requestor) + (REQ_count - 1) * tCCD + tRTW + 1 + tWL + tBUS;
								}
							}
							else if (cmd->busPacketType == WR && cmd->requestorID == requestor)
							{
								temp_timer[requestor] = tWL + tBUS;
							}
						}
					}
				}
				else if (req_statues_flag[requestor] == false)
				{

					if (reqNotEmpty(requestor))
					{

						req_statues_flag[requestor] = true;
						if (returnOldest(requestor)->busPacketType == RD)
						{
							req_open[requestor] = true;
							if (cmd == NULL)
							{
								if (isReadyTimer(returnOldest(requestor), requestor) <= 1)
								{
									if (CAStimer == 0)
										temp_timer[requestor] = 1 + front_has_rd * tCCD + tRL + tBUS;
									else
										temp_timer[requestor] = CAStimer + front_has_rd * tCCD + tRL + tBUS;
								}
								else
								{
									temp_timer[requestor] = isReadyTimer(returnOldest(requestor), requestor) + (front_has_rd - 2) * tCCD + max(tRTW, 2 * tCCD) + tWtoR - 1 + tRL + tBUS;
								}
							}
							else if (cmd->busPacketType == PRE && cmd->requestorID != requestor)
							{
								if (isReadyTimer(returnOldest(requestor), requestor) <= 1)
								{
									if (CAStimer == 0)
										temp_timer[requestor] = 1 + front_has_rd * tCCD + tRL + tBUS;
									else
										temp_timer[requestor] = CAStimer + front_has_rd * tCCD + tRL + tBUS;
								}
								else
								{
									temp_timer[requestor] = isReadyTimer(returnOldest(requestor), requestor) + (front_has_rd - 2) * tCCD + max(tRTW, 2 * tCCD) + tWtoR - 1 + tRL + tBUS;
								}
							}
							else if (cmd->busPacketType == ACT && cmd->requestorID != requestor)
							{
								if (isReadyTimer(returnOldest(requestor), requestor) <= 1)
								{
									if (CAStimer == 0)
										temp_timer[requestor] = 1 + front_has_rd * tCCD + tRL + tBUS;
									else
										temp_timer[requestor] = CAStimer + front_has_rd * tCCD + tRL + tBUS;
								}
								else
								{
									temp_timer[requestor] = isReadyTimer(returnOldest(requestor), requestor) + (front_has_rd - 2) * tCCD + max(tRTW, 2 * tCCD) + tWtoR - 1 + tRL + tBUS;
								}
							}
							else if (cmd->busPacketType == RD && cmd->requestorID != requestor)
							{
								if (isReadyTimer(returnOldest(requestor), requestor) <= 1)
								{
									temp_timer[requestor] = tCCD + front_has_rd * tCCD + tRL + tBUS;
								}
								else
								{
									temp_timer[requestor] = isReadyTimer(returnOldest(requestor), requestor) + (front_has_rd - 2) * tCCD + max(tRTW, 2 * tCCD) + tWtoR - 1 + tRL + tBUS;
								}
							}
							else if (cmd->busPacketType == WR && cmd->requestorID != requestor)
							{
								if (isReadyTimer(returnOldest(requestor), requestor) <= 1)
								{
									temp_timer[requestor] = tWtoR + front_has_rd * tCCD + tRL + tBUS;
								}
								else
								{
									temp_timer[requestor] = isReadyTimer(returnOldest(requestor), requestor) + (front_has_rd - 2) * tCCD + max(tRTW, 2 * tCCD) + tWtoR - 1 + tRL + tBUS;
								}
							}
							else if (cmd->busPacketType == RD && cmd->requestorID == requestor)
							{
								temp_timer[requestor] = tRL + tBUS;
							}
						}
						else if (returnOldest(requestor)->busPacketType == WR)
						{
							req_open[requestor] = true;
							if (cmd == NULL)
							{
								if (isReadyTimer(returnOldest(requestor), requestor) <= 1)
								{
									if (CAStimer == 0)
										temp_timer[requestor] = 1 + (REQ_count - 1) * tCCD + tRTW + tWL + tBUS;
									else
										temp_timer[requestor] = CAStimer + (REQ_count - 1) * tCCD + tRTW + tWL + tBUS;
								}
								else
								{
									temp_timer[requestor] = isReadyTimer(returnOldest(requestor), requestor) + (REQ_count - 1) * tCCD + tRTW + 1 + tWL + tBUS;
								}
							}
							else if (cmd->busPacketType == PRE && cmd->requestorID != requestor)
							{
								if (isReadyTimer(returnOldest(requestor), requestor) <= 1)
								{
									if (CAStimer == 0)
										temp_timer[requestor] = 1 + (REQ_count - 1) * tCCD + tRTW + tWL + tBUS;
									else
										temp_timer[requestor] = CAStimer + (REQ_count - 1) * tCCD + tRTW + tWL + tBUS;
								}
								else
								{
									temp_timer[requestor] = isReadyTimer(returnOldest(requestor), requestor) + (REQ_count - 1) * tCCD + tRTW + 1 + tWL + tBUS;
								}
							}
							else if (cmd->busPacketType == ACT && cmd->requestorID != requestor)
							{
								if (isReadyTimer(returnOldest(requestor), requestor) <= 1)
								{
									if (CAStimer == 0)
										temp_timer[requestor] = 1 + (REQ_count - 1) * tCCD + tRTW + tWL + tBUS;
									else
										temp_timer[requestor] = CAStimer + (REQ_count - 1) * tCCD + tRTW + tWL + tBUS;
								}
								else
								{
									temp_timer[requestor] = isReadyTimer(returnOldest(requestor), requestor) + (REQ_count - 1) * tCCD + tRTW + 1 + tWL + tBUS;
								}
							}
							else if (cmd->busPacketType == RD && cmd->requestorID != requestor)
							{
								if (isReadyTimer(returnOldest(requestor), requestor) <= 1)
								{
									temp_timer[requestor] = tRTW + (REQ_count - 1) * tCCD + tRTW + tWL + tBUS;
								}
								else
								{
									temp_timer[requestor] = isReadyTimer(returnOldest(requestor), requestor) + (REQ_count - 1) * tCCD + tRTW + 1 + tWL + tBUS;
								}
							}
							else if (cmd->busPacketType == WR && cmd->requestorID != requestor)
							{
								if (isReadyTimer(returnOldest(requestor), requestor) <= 1)
								{
									temp_timer[requestor] = tCCD + (REQ_count - 1) * tCCD + tRTW + tWL + tBUS;
								}
								else
								{
									temp_timer[requestor] = isReadyTimer(returnOldest(requestor), requestor) + (REQ_count - 1) * tCCD + tRTW + 1 + tWL + tBUS;
								}
							}
							else if (cmd->busPacketType == WR && cmd->requestorID == requestor)
							{
								temp_timer[requestor] = tWL + tBUS;
							}
						}
						else if (returnOldest(requestor)->busPacketType == PRE) // done modifications
						{
							if (cmd != NULL)
							{
								if (cmd->requestorID == requestor)
								{
									if (cmd->busPacketType == RD)
									{
										req_open[requestor] = false;
										temp_timer[requestor] = max(isReadyTimer(returnOldest(requestor), requestor), tRTP) +
																LPRE[front_has_pre] + tRP + LACT[front_has_act] + tRCD + (front_has_rd - 2) * tCCD + max(tRTW, 2 * tCCD) + tWtoR - 1 + tRL + tBUS;
									}
									if (cmd->busPacketType == WR)
									{
										req_open[requestor] = false;
										temp_timer[requestor] = max(isReadyTimer(returnOldest(requestor), requestor), tWR) + LPRE[front_has_pre] + tRP + LACT[front_has_act] + tRCD + (front_has_rd - 2) * tCCD + max(tRTW, 2 * tCCD) + tWtoR - 1 + tRL + tBUS;
									}
									else
									{
										req_open[requestor] = false;
										temp_timer[requestor] = tRP + LACT[front_has_act] + tRCD + (front_has_rd - 2) * tCCD + max(tRTW, 2 * tCCD) + tWtoR - 1 + tRL + tBUS;
									}
								}
								else
								{
									req_open[requestor] = false;
									temp_timer[requestor] = isReadyTimer(returnOldest(requestor), requestor) + LPRE[front_has_pre] + tRP + LACT[front_has_act] + tRCD + (front_has_rd - 2) * tCCD + max(tRTW, 2 * tCCD) + tWtoR - 1 + tRL + tBUS;
								}
							}
							else
							{
								req_open[requestor] = false;
								if (isReadyTimer(returnOldest(requestor), requestor) > 0)
									temp_timer[requestor] = isReadyTimer(returnOldest(requestor), requestor) + LPRE[front_has_pre] + tRP + LACT[front_has_act] + tRCD + (front_has_rd - 2) * tCCD + max(tRTW, 2 * tCCD) + tWtoR - 1 + tRL + tBUS;
								else if (isReadyTimer(returnOldest(requestor), requestor) == 0)
									temp_timer[requestor] = isReadyTimer(returnOldest(requestor), requestor) + LPRE[front_has_pre] + tRP + LACT[front_has_act] + tRCD + (front_has_rd - 2) * tCCD + max(tRTW, 2 * tCCD) + tWtoR - 1 + 1 + tRL + tBUS;
							}
						}
					}
					else
					{
						temp_timer[requestor] = 0;
					}
				}
				tempLatency = temp_timer[requestor];
			}
			return tempLatency;
		}

		BusPacket *commandSchedule()
		{

			scheduledCommand = NULL;

			tRL = getTiming("tRL");
			tBUS = getTiming("tBus");
			tRTW = getTiming("tRTW");
			tWTR = getTiming("tWTR");
			tRRD = getTiming("tRRD");
			tRP = getTiming("tRP");
			tRCD = getTiming("tRCD");
			tWR = getTiming("tWR");
			tWL = getTiming("tWL");
			tRTP = getTiming("tRTP");
			tWtoR = tBUS + tWTR + tWL;

			// send the command decided in the previous cycle from the register and remove from both queues
			if (mode == "RTA" && clock != 1)
			{
				if (scheduledCommand_RTA != NULL)
				{
					rta_count++;
					scheduledCommand = scheduledCommand_RTA;
					// if(scheduledCommand->busPacketType == RD)
					// 	cout<<"%%%%%%%%%%%%%%%%%%%%%%%%% RTA RD"<<"\t"<<clock<<":"<<"\t\tAddress: "<<scheduledCommand->address<<"\tBank: "<<scheduledCommand->bank<<"\t\tColumn: "<<scheduledCommand->column<<"\tRow: "<<scheduledCommand->row<<endl;
					// else if(scheduledCommand->busPacketType == WR)
					// 	cout<<"%%%%%%%%%%%%%%%%%%%%%%%%% RTA WR"<<"\t"<<clock<<":"<<"\t\tAddress: "<<scheduledCommand->address<<"\tBank: "<<scheduledCommand->bank<<"\t\tColumn: "<<scheduledCommand->column<<"\tRow: "<<scheduledCommand->row<<endl;
					// else if(scheduledCommand->busPacketType == ACT)
					// 	cout<<"%%%%%%%%%%%%%%%%%%%%%%%%% RTA ACT"<<"\t"<<clock<<":"<<"\t\tAddress: "<<scheduledCommand->address<<"\tBank: "<<scheduledCommand->bank<<"\t\tColumn: "<<scheduledCommand->column<<"\tRow: "<<scheduledCommand->row<<endl;
					// else if(scheduledCommand->busPacketType == PRE)
					// 	cout<<"%%%%%%%%%%%%%%%%%%%%%%%%% RTA PRE"<<"\t"<<clock<<":"<<"\t\tAddress: "<<scheduledCommand->address<<"\tBank: "<<scheduledCommand->bank<<"\t\tColumn: "<<scheduledCommand->column<<"\tRow: "<<scheduledCommand->row<<endl;
					
					if (scheduledCommand->busPacketType < ACT)
					{

						servedFlags[scheduledCommand->bank] = true;
						req_statues_flag[scheduledCommand->requestorID] = false;
					}

					queuePending[scheduledCommand->bank] = false;

					sendCommand(scheduledCommand, scheduledCommand->bank, false);
					commandRegisters.erase(commandRegisters.begin() + registerIndex);
				}
			}
			else if (mode == "HPA" && clock != 1)
			{

				hpa_count++;
				if (scheduledCommand_HPA != NULL)
				{

					scheduledCommand = scheduledCommand_HPA;

					// if(scheduledCommand->busPacketType == RD)
					// 	cout<<"%%%%%%%%%%%%%%%%%%%%%%%%% HPA RD"<<"\t"<<clock<<":"<<"\t\tAddress: "<<scheduledCommand->address<<"\tBank: "<<scheduledCommand->bank<<"\t\tColumn: "<<scheduledCommand->column<<"\tRow: "<<scheduledCommand->row<<endl;
					// else if(scheduledCommand->busPacketType == WR)
					// 	cout<<"%%%%%%%%%%%%%%%%%%%%%%%%% HPA WR"<<"\t"<<clock<<":"<<"\t\tAddress: "<<scheduledCommand->address<<"\tBank: "<<scheduledCommand->bank<<"\t\tColumn: "<<scheduledCommand->column<<"\tRow: "<<scheduledCommand->row<<endl;
					// else if(scheduledCommand->busPacketType == ACT)
					// 	cout<<"%%%%%%%%%%%%%%%%%%%%%%%%% HPA ACT"<<"\t"<<clock<<":"<<"\t\tAddress: "<<scheduledCommand->address<<"\tBank: "<<scheduledCommand->bank<<"\t\tColumn: "<<scheduledCommand->column<<"\tRow: "<<scheduledCommand->row<<endl;
					// else if(scheduledCommand->busPacketType == PRE)
					//  	cout<<"%%%%%%%%%%%%%%%%%%%%%%%%% HPA PRE"<<"\t"<<clock<<":"<<"\t\tAddress: "<<scheduledCommand->address<<"\tBank: "<<scheduledCommand->bank<<"\t\tColumn: "<<scheduledCommand->column<<"\tRow: "<<scheduledCommand->row<<endl;

					if (scheduledCommand->busPacketType < ACT)
					{
						req_statues_flag[scheduledCommand->requestorID] = false;
					}

					sendCommand(scheduledCommand, scheduledCommand->bank, false);
				}
			}

			setDeadline();

			for (unsigned int i = 0; i < REQ_count; i++)
			{
				if (service_deadline[i] > 0)
					service_deadline[i]--;
				else
					service_deadline[i] = 0;
			}

			checkCommand = NULL;
			checkCommand_temp_1 = NULL;
			checkCommand_temp_2 = NULL;
			scheduledCommand_HPA = NULL;
			scheduledCommand_RTA = NULL;

			/////////////////////////////////////////////////////////////// HPA ///////////////////////////////////////////////////////////////

			for (unsigned int index = 0; index < commandQueue.size(); index++)
			{
				if (commandQueue[index]->isPerRequestor())
				{
					if (commandQueue[index]->getRequestorIndex() > 0) // There is more than 0 requestor in the design
					{
						for (unsigned int num = 0; num < commandQueue[index]->getRequestorIndex(); num++) // For all requestors from "num". getRequestorIndex() gives the number of requestors
						{
							if (commandQueue[index]->getRequestorSize(num, false) > 0) // Return the buffer size of the requestor
							{
								if (checkCommand_temp_1 != NULL)
								{
									checkCommand = commandQueue[index]->getRequestorCommand(num, false);
									if (checkCommand != NULL)
									{
										if (checkCommand->busPacketType == RD || checkCommand->busPacketType == WR)
										{
											if (isReady(checkCommand, index))
											{
												if (isIssuable(checkCommand))
												{
													if (checkCommand->arriveTime < checkCommand_temp_1->arriveTime)
													{
														checkCommand_temp_1 = checkCommand;
													}
												}
											}
										}
									}
									checkCommand = NULL;
								}
								else
								{
									checkCommand = commandQueue[index]->getRequestorCommand(num, false);
									if (checkCommand != NULL)
									{
										if (checkCommand->busPacketType == RD || checkCommand->busPacketType == WR)
										{
											if (isReady(checkCommand, index))
											{
												if (isIssuable(checkCommand))
												{
													checkCommand_temp_1 = checkCommand;
													break;
												}
											}
										}
									}
									checkCommand = NULL;
								}
							}
						}
						if (checkCommand_temp_1 != NULL)
						{
							for (unsigned int num = 0; num < commandQueue[index]->getRequestorIndex(); num++) // For all requestors from "num". getRequestorIndex() gives the number of requestors
							{
								if (commandQueue[index]->getRequestorSize(num, false) > 0) // Return the buffer size of the requestor
								{
									checkCommand = commandQueue[index]->getRequestorCommand(num, false);
									if (checkCommand != NULL)
									{
										if (checkCommand->busPacketType == RD || checkCommand->busPacketType == WR)
										{
											if (isReady(checkCommand, index))
											{
												if (isIssuable(checkCommand))
												{
													if (checkCommand->arriveTime < checkCommand_temp_1->arriveTime)
													{
														checkCommand_temp_1 = checkCommand;
													}
												}
											}
										}
									}
									checkCommand = NULL;
								}
							}
						}
					}
				}
			}
			if (checkCommand_temp_1 != NULL)
			{
				scheduledCommand_HPA = checkCommand_temp_1;
			}
			else
			{
				checkCommand = NULL;
				checkCommand_temp_1 = NULL;
				for (unsigned int index = 0; index < commandQueue.size(); index++)
				{
					if (commandQueue[index]->isPerRequestor())
					{
						if (commandQueue[index]->getRequestorIndex() > 0) // There is more than 0 requestor in the design
						{
							for (unsigned int num = 0; num < commandQueue[index]->getRequestorIndex(); num++) // For all requestors from "num". getRequestorIndex() gives the number of requestors
							{
								if (commandQueue[index]->getRequestorSize(num, false) > 0) // Return the buffer size of the requestor
								{
									if (checkCommand_temp_1 != NULL)
									{
										checkCommand = commandQueue[index]->getRequestorCommand(num, false);
										if (checkCommand != NULL)
										{
											if (checkCommand->busPacketType == ACT || checkCommand->busPacketType == PRE)
											{
												if (isReady(checkCommand, index))
												{
													if (isIssuable(checkCommand))
													{
														if (checkCommand->arriveTime < checkCommand_temp_1->arriveTime)
														{
															checkCommand_temp_1 = checkCommand;
														}
													}
												}
											}
										}
										checkCommand = NULL;
									}
									else
									{
										checkCommand = commandQueue[index]->getRequestorCommand(num, false);
										if (checkCommand != NULL)
										{
											if (checkCommand->busPacketType == ACT || checkCommand->busPacketType == PRE)
											{
												if (isReady(checkCommand, index))
												{
													if (isIssuable(checkCommand))
													{
														checkCommand_temp_1 = checkCommand;
													}
												}
											}
										}
										checkCommand = NULL;
									}
								}
							}
							for (unsigned int num = 0; num < commandQueue[index]->getRequestorIndex(); num++) // For all requestors from "num". getRequestorIndex() gives the number of requestors
							{
								if (commandQueue[index]->getRequestorSize(num, false) > 0) // Return the buffer size of the requestor
								{
									checkCommand = commandQueue[index]->getRequestorCommand(num, false);
									if (checkCommand != NULL)
									{
										if (checkCommand->busPacketType == ACT || checkCommand->busPacketType == PRE)
										{
											if (isReady(checkCommand, index))
											{
												if (isIssuable(checkCommand))
												{
													if (checkCommand->arriveTime < checkCommand_temp_1->arriveTime)
													{
														checkCommand_temp_1 = checkCommand;
													}
												}
											}
										}
									}
									checkCommand = NULL;
								}
							}
						}
					}
				}
			}

			if (checkCommand_temp_1 != NULL)
			{
				scheduledCommand_HPA = checkCommand_temp_1;
			}

			checkCommand = NULL;
			checkCommand_temp_1 = NULL;
			checkCommand_temp_2 = NULL;

			/////////////////////////////////////////////////////////////// Decision needs to be made now ///////////////////////////////////////////////////////////////
			mode_prev = mode;  // keep the prevous mode to further use of fixing the queues
			if (mode == "HPA") // if HPMode
			{

				for (unsigned int index = 0; index < REQ_count; index++)
				{

					if (WCLator(index, scheduledCommand_HPA) >= service_deadline[index])
					{
						cout<<"-----------------------------------------SWITCH TO RTA-------------------------------------------- Becaus of "<<index<<" latency "<<WCLator(index,scheduledCommand_HPA)<<endl;
						suspect_requestor = index;
						suspect_flag = true;
						mode = "RTA";
						break;
					}
				}
			}
			else // if RTMode
			{

				for (unsigned int index = 0; index < REQ_count; index++)
				{

					if (WCLator(index, scheduledCommand_HPA) < service_deadline[index])
					{
						cout<<"-----------------------------------------SWITCH TO RTA--------------------------------------------Becaus of "<<index<<" latency "<<WCLator(index,scheduledCommand_HPA)<<endl;
						mode = "HPA";
					}
					else
					{
						//cout<<"-----------------------------------------REMAIN TO RTA--------------------------------------------Becaus of "<<index<<" latency "<<WCLator(index,scheduledCommand_HPA)<<endl;
						mode = "RTA";
						break;
					}
				}
			}
			mode = "HPA";

			/////////////////////////////////////////////////////////////// RTA ///////////////////////////////////////////////////////////////
			scheduledCommand_RTA = NULL;

			/*
			cout<<"||||||||||||||||||||||||||||| Order Watch ||||||||||||||||||||||||"<<endl;
			if(Order.size() != 0)
			{
				for(unsigned int h=0; h<Order.size() ; h++){
					cout<<" Order watch:  "<<Order.at(h)<<endl; 					
				}
			}
			*/

			if (mode == "RTA")
			{
				for (unsigned int index = 0; index < commandQueue_RT.size(); index++) // Scan the command queue for ready CAS
				{
					for (unsigned int num = 0; num < commandQueue_RT[index]->getRequestorIndex(); num++) // For all requestors from "num". getRequestorIndex() gives the number of requestors
					{
						checkCommand = NULL;

						if (!(isShared(index)))
						{
							if (commandQueue_RT[index]->getRequestorSize(num, true) > 0 && queuePending[index] == false)
							{
								checkCommand = commandQueue_RT[index]->getRequestorCommand(num, true);
							}
							if (checkCommand != NULL)
							{
								if (isReady(checkCommand, index))
								{

									commandRegisters.push_back(make_pair(checkCommand, checkCommand->bank));
									queuePending[index] = true;
								}
							}
						}
						else
						{
							if (commandQueue_RT[index]->getRequestorSize(num, true) > 0 && queuePendingShared[num] == false)
							{
								checkCommand = commandQueue_RT[index]->getRequestorCommand(num, true);
							}
							if (checkCommand != NULL)
							{
								if (isReady(checkCommand, index))
								{
									////cout<<"the command that is going to push in command register is  "<<checkCommand->busPacketType<<"   from   "<<checkCommand->requestorID<<endl;
									commandRegisters.push_back(make_pair(checkCommand, checkCommand->bank));
									queuePendingShared[num] = true;
								}
							}
						}
					}
				}

				bool CAS_remain = false;
				bool newRound = true;		  // Should reset servedFlag?
				bool switch_expected = false; // A different CAS can be schedule

				if (!commandRegisters.empty())
				{

					for (unsigned int index = 0; index < commandRegisters.size(); index++)
					{
						checkCommand = commandRegisters[index].first;

						if (checkCommand->busPacketType < ACT)
						{
							bool isServed = true;

							isServed = servedFlags[commandRegisters[index].second];
							if (isServed == false && checkCommand->busPacketType == roundType && !(isBlocked(checkCommand)))
							{
								if (checkCommand->address == returnOldest(checkCommand->requestorID)->address && checkCommand->busPacketType == returnOldest(checkCommand->requestorID)->busPacketType)
								{

									CAS_remain = true;
									break;
								}
							}
							else if (isServed == false && checkCommand->busPacketType != roundType)
							{
								if (checkCommand->address == returnOldest(checkCommand->requestorID)->address && checkCommand->busPacketType == returnOldest(checkCommand->requestorID)->busPacketType)
								{
									switch_expected = true;
								}
							}
						}
					}
					if (CAS_remain == true)
					{

						newRound = false;
					}
					else
					{
						if (switch_expected == true)
						{
							newRound = true;
						}
					}

					if (newRound && count == 0)
					{
						for (unsigned int index = 0; index < servedFlags.size(); index++)
						{
							servedFlags[index] = false;
						}
						if (switch_expected == true)
						{

							if (roundType == WR)
							{
								roundType = RD;
							}
							else
							{
								roundType = WR;
							}
						}
						else
						{
						}
					}

					checkCommand = NULL;
				}

				registerIndex = 0;
				// First Level of Arbitration among oldest of each requestor
				if (!commandRegisters.empty())
				{
					for (unsigned int loop = 0; loop < Order.size(); loop++)
					{
						if (scheduledCommand_RTA == NULL)
						{
							for (unsigned int index = 0; index < commandRegisters.size(); index++)
							{
								checkCommand = commandRegisters[index].first;
								if (servedFlags[checkCommand->bank] == false)
								{
									if (checkCommand->busPacketType == roundType)
									{
										if (checkCommand->requestorID == Order.at(loop))
										{
											if (isIssuable(checkCommand) && !(isBlocked(checkCommand)))
											{
												if (checkCommand->address == returnOldest(checkCommand->requestorID)->address && checkCommand->busPacketType == returnOldest(checkCommand->requestorID)->busPacketType)
												{
													if (active_shared_exist == false || !(isShared(checkCommand->bank)))
													{
														scheduledCommand_RTA = checkCommand;
														registerIndex = index;
														break;
													}
													else
													{
														if ((checkCommand->requestorID == active_shared_req) && (checkCommand->bank == active_shared_bank))
														{
															scheduledCommand_RTA = checkCommand;
															registerIndex = index;
															break;
														}
													}
												}
											}
										}
									}
								}
								checkCommand = NULL;
							}
						}
					}
				}

				// Second Level of Arbitration among non oldest of each requestor
				if (scheduledCommand_RTA == NULL)
				{
					// Essentially if happens here anything, it means that a new round has been started
					if (!commandRegisters.empty())
					{
						for (unsigned int loop = 0; loop < Order.size(); loop++)
						{
							if (scheduledCommand_RTA == NULL)
							{
								for (unsigned int index = 0; index < commandRegisters.size(); index++)
								{
									checkCommand = commandRegisters[index].first;
									if (servedFlags[checkCommand->bank] == false)
									{
										if (checkCommand->busPacketType == roundType)
										{
											if (checkCommand->requestorID == Order.at(loop))
											{
												if (isIssuable(checkCommand) && !(isBlocked(checkCommand)))
												{

													if (active_shared_exist == false || !(isShared(checkCommand->bank)))
													{
														scheduledCommand_RTA = checkCommand;
														registerIndex = index;
														for (unsigned int index = 0; index < servedFlags.size(); index++) // if any non oldest CAS goes, the round is reset
															servedFlags[index] = false;
														break;
													}
													else
													{
														if ((checkCommand->requestorID == active_shared_req) && (checkCommand->bank == active_shared_bank))
														{
															scheduledCommand_RTA = checkCommand;
															registerIndex = index;
															for (unsigned int index = 0; index < servedFlags.size(); index++) // if any non oldest CAS goes, the round is reset
																servedFlags[index] = false;
															break;
														}
													}
												}
											}
										}
									}
									checkCommand = NULL;
								}
							}
						}
					}
				}

				// Round-Robin ACT arbitration
				if (scheduledCommand_RTA == NULL && !commandRegisters.empty())
				{
					for (unsigned int loop = 0; loop < Order.size(); loop++)
					{
						for (unsigned int index = 0; index < commandRegisters.size(); index++)
						{
							if (commandRegisters[index].first->busPacketType == ACT)
							{
								checkCommand = commandRegisters[index].first;
								if (isIssuable(checkCommand) && checkCommand->requestorID == Order.at(loop))
								{
									if (!isBlocked(checkCommand))
									{
										if (!(isShared(checkCommand->bank)))
										{
											scheduledCommand_RTA = checkCommand;
											registerIndex = index;
											break;
										}
										else
										{
											if ((checkCommand->requestorID == active_shared_req) && (checkCommand->bank == active_shared_bank))
											{
												scheduledCommand_RTA = checkCommand;
												registerIndex = index;
												break;
											}
										}
									}
								}
								checkCommand = NULL;
							}
						}
					}
				}

				// Round-Robin PRE arbitration
				if (scheduledCommand_RTA == NULL && !commandRegisters.empty())
				{
					for (unsigned int loop = 0; loop < Order.size(); loop++)
					{

						for (unsigned int index = 0; index < commandRegisters.size(); index++)
						{

							if (commandRegisters[index].first->busPacketType == PRE)
							{

								checkCommand = commandRegisters[index].first;
								if (isIssuable(checkCommand) && checkCommand->requestorID == Order.at(loop))
								{
									if (!isBlocked(checkCommand))
									{
										scheduledCommand_RTA = checkCommand;
										registerIndex = index;
										break;
									}
								}
								checkCommand = NULL;
							}
						}
					}
				}
			}

			if (mode == "RTA") // if RTA/HPA was previous and now RTA, and it tries to close a bank, the HPA queue should be updated
			{

				if (scheduledCommand_RTA != NULL)
				{

					if (scheduledCommand_RTA->busPacketType == PRE)
					{

						if (commandQueue[scheduledCommand_RTA->bank]->getRequestorSize(scheduledCommand_RTA->requestorID, false) > 0)
						{
							unsigned int temp_size = commandQueue[scheduledCommand_RTA->bank]->getRequestorSize(scheduledCommand_RTA->requestorID, false);
							for (unsigned int l = 0; l < temp_size; l++)
							{
								commandQueue[scheduledCommand_RTA->bank]->removeCommand(scheduledCommand_RTA->requestorID, false);
							}
						}

						for (unsigned int l = 0; l < commandQueue_RT[scheduledCommand_RTA->bank]->getRequestorSize(scheduledCommand_RTA->requestorID, true); l++)
						{

							BusPacket *checkCommand_copy;
							checkCommand_copy = NULL;
							checkCommand_copy = commandQueue_RT[scheduledCommand_RTA->bank]->getRequestorCommand_position(scheduledCommand_RTA->requestorID, l, true);

							commandQueue[scheduledCommand_RTA->bank]->insertCommand(checkCommand_copy, true, false);
						}

						requestQueues_local[scheduledCommand_RTA->bank]->syncRequest(scheduledCommand_RTA->requestorID, 0);
					}
				}
			}
			if (mode_prev == "RTA" && mode == "HPA") // remove anything remained in the RT command registers if going to HPA
			{
				commandRegisters.clear();
				for (unsigned int y = 0; y < REQ_count; y++)
				{
					queuePending[y] = false;
				}
				for (unsigned int index = 0; index < servedFlags.size(); index++)
				{
					servedFlags[index] = false;
				}
				roundType = BusPacketType::RD; // Read Type
			}
			if (mode == "HPA")
			{

				if (scheduledCommand_HPA != NULL)
				{

					if (scheduledCommand_HPA->busPacketType == RD || scheduledCommand_HPA->busPacketType == WR)
					{

						requestQueues_local[0]->updateRowTable(scheduledCommand_HPA->rank, scheduledCommand_HPA->bank, scheduledCommand_HPA->row);

						if (returnOldest(scheduledCommand_HPA->requestorID)->busPacketType == scheduledCommand_HPA->busPacketType && returnOldest(scheduledCommand_HPA->requestorID)->address == scheduledCommand_HPA->address)
						{
							wcl = clock - deadline_track[scheduledCommand_HPA->requestorID];

							if (scheduledCommand_HPA->busPacketType == RD)
							{

								if (req_wcl_index == 100000)
								{
									cout << "done" << endl;
									abort();
								}
							}
							else if (scheduledCommand_HPA->busPacketType == WR)
							{
								if (req_wcl_index == 100000)
								{
									cout << "done" << endl;
									abort();
								}
							}
							for (unsigned int h = 0; h < Order.size(); h++)
							{
								if (Order.at(h) == scheduledCommand_HPA->requestorID)
								{
									Order.erase(Order.begin() + h);
									break;
								}
							}
							deadline_set_flag[scheduledCommand_HPA->requestorID] = false;
						}

						commandQueue[scheduledCommand_HPA->bank]->removeCommand(scheduledCommand_HPA->requestorID, false);

						if (commandQueue_RT[scheduledCommand_HPA->bank]->getRequestorSize(scheduledCommand_HPA->requestorID, true) > 0)
						{

							if (scheduledCommand_HPA->address == commandQueue_RT[scheduledCommand_HPA->bank]->getRequestorCommand(scheduledCommand_HPA->requestorID, true)->address && scheduledCommand_HPA->busPacketType == commandQueue_RT[scheduledCommand_HPA->bank]->getRequestorCommand(scheduledCommand_HPA->requestorID, true)->busPacketType)
							{

								commandQueue_RT[scheduledCommand_HPA->bank]->removeCommand(scheduledCommand_HPA->requestorID, true);
							}
						}

						requestQueues_local[scheduledCommand_HPA->bank]->removeRequest(false, scheduledCommand_HPA->requestorID);
					}
				}
			}
			else
			{

				if (scheduledCommand_RTA != NULL)
				{
					if (scheduledCommand_RTA->busPacketType == RD || scheduledCommand_RTA->busPacketType == WR)
					{

						if (returnOldest(scheduledCommand_RTA->requestorID)->busPacketType == scheduledCommand_RTA->busPacketType && returnOldest(scheduledCommand_RTA->requestorID)->address == scheduledCommand_RTA->address)
						{
							for (unsigned int h = 0; h < Order.size(); h++)
							{
								if (Order.at(h) == scheduledCommand_RTA->requestorID)
								{
									wcl = clock - deadline_track[scheduledCommand_RTA->requestorID];

									if (scheduledCommand_RTA->busPacketType == RD)
									{
										if (req_wcl_index == 100000)
										{
											cout << "done" << endl;
											abort();
										}
									}
									else if (scheduledCommand_RTA->busPacketType == WR)
									{
										if (req_wcl_index == 100000)
										{
											cout << "done" << endl;
											abort();
										}
									}
									Order.erase(Order.begin() + h);
									break;
								}
							}
							deadline_set_flag[scheduledCommand_RTA->requestorID] = false;
						}
						requestQueues_local[0]->updateRowTable(scheduledCommand_RTA->rank, scheduledCommand_RTA->bank, scheduledCommand_RTA->row);

						if (commandQueue[scheduledCommand_RTA->bank]->getRequestorSize(scheduledCommand_RTA->requestorID, false) > 0)
						{
							if (scheduledCommand_RTA->address == commandQueue[scheduledCommand_RTA->bank]->getRequestorCommand(scheduledCommand_RTA->requestorID, false)->address && scheduledCommand_RTA->busPacketType == commandQueue[scheduledCommand_RTA->bank]->getRequestorCommand(scheduledCommand_RTA->requestorID, false)->busPacketType)
								commandQueue[scheduledCommand_RTA->bank]->removeCommand(scheduledCommand_RTA->requestorID, false);
						}

						commandQueue_RT[scheduledCommand_RTA->bank]->removeCommand(scheduledCommand_RTA->requestorID, true);

						requestQueues_local[scheduledCommand_RTA->bank]->removeRequest(true, scheduledCommand_RTA->requestorID);
					}
				}
			}

			wcl = 0;
			return scheduledCommand;
		}
	};
}
#endif /*  */
