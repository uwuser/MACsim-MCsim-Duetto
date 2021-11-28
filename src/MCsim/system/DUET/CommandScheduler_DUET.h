
#ifndef COMMANDSCHEDULER_DUET_H
#define COMMANDSCHEDULER_DUET_H

#include "../../src/CommandScheduler.h"

namespace MCsim
{
	class CommandScheduler_DUET : public CommandScheduler
	{
	private:
		vector<RequestQueue *> 						 &requestQueues_local;
		vector<bool> 								 servedFlags;
		vector<bool> 								 queuePending;
		vector<bool> 								 queuePendingShared;
		vector<std::pair<BusPacket *, unsigned int>> commandRegisters;
		vector<unsigned int> 						 Order;
		vector<unsigned int> 						 OrderTemp;
		vector<bool> 								 sharedBank;

		std::map<unsigned int, unsigned int> service_deadline;
		std::map<unsigned int, unsigned int> deadline_track;
		std::map<unsigned int, unsigned int> temp_timer;

		string mode;	  //RTA, HPA
		string mode_prev; //RTA, HPA
		string impl;

		int N;
		int front_has_pre;
		int front_has_act;
		int front_has_rd;
		int front_has_wr;
		int suspect_requestor;

		unsigned int registerIndex;
		unsigned int temp;
		unsigned int active_shared_req;
		unsigned int active_shared_bank;
		unsigned int bank_count;
		unsigned int REQ_count;
		unsigned int k;
		unsigned int init_deadline;
		unsigned int init_deadline_CR;
		unsigned int init_deadline_CW;
		unsigned int init_deadline_OR;
		unsigned int init_deadline_OW;
		unsigned int roundType;
		unsigned int CInterR;
		unsigned int CInterW;

		unsigned int RR;
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

		unsigned long int hpa_count;
		unsigned long int rta_count;

		unsigned long long count;
		unsigned long long req_wcl_index;

		bool active_shared_exist;
		bool deadline_set_flag[8];
		bool req_open[8];
		bool req_statues_flag[8];
		bool suspect_flag;
		bool remove;

	public:
		CommandScheduler_DUET(vector<RequestQueue *> &requestQueues, vector<CommandQueue *> &commandQueues,
							  const map<unsigned int, bool> &requestorTable, vector<CommandQueue *> &commandQueue_RT) : CommandScheduler(commandQueues, requestorTable, commandQueue_RT),
																														requestQueues_local(requestQueues)
		{

			roundType = BusPacketType::RD; // Read Type			
			CInterR      	 = 4;
			CInterW      	 = 4;
			front_has_pre  	 = 0;
			front_has_act    = 0;
			front_has_rd 	 = 0;
			front_has_wr 	 = 0;
			tCCD 		 	 = 4;
			REQ_count	 	 = 8;									/***** SHOULD  BE CONFIGURED *****/
			count		  	 = 0;
			suspect_flag  	 = false;
			N 			  	 = 0;
			bank_count 	  	 = 8;		 							/***** SHOULD  BE CONFIGURED *****/
			init_deadline 	 = 156; 								/***** SHOULD  BE CONFIGURED *****/
			init_deadline_CR = 156;
			init_deadline_CW = 156;
			init_deadline_OR = 156;
			init_deadline_OW = 156;
			mode = "HPA";
			impl = "B";
			active_shared_req   = 1000;
			active_shared_bank  = 1000;
			active_shared_exist = false;
			//sharedBank.push_back();  								/***** SHOULD  BE CONFIGURED *****/
			//sharedBank.push_back();	     						/***** SHOULD  BE CONFIGURED *****/
			//sharedBank.push_back();  						 		/***** SHOULD  BE CONFIGURED *****/
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
		}

		/**
			Return the oldest command of the requestor reqID.
			@param reqID The requestor ID.			
		*/
		BusPacket *returnOldest(unsigned int reqID)
		{			
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
		
		/**
			Return true if at least one command exists for reqID.
			@param reqID The requestor ID.			
		*/
		bool reqNotEmpty(unsigned int reqID)
		{			
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
		
		/**
			Return true ff there is another oldest request to the same bank.
			@param reqID The requestor ID.			
		*/
		bool isBlocked(BusPacket *cmd)
		{			
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
		
		/**
			Return true if the bank is shared among requestors.
			@param index The index of bank.			
		*/
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
		
		/**
			Set the deadline to each requestor in the system.
			This represents the deadline for the oldest request of each requestor.
			It works only on the RT command queue obviously.					
		*/
		void setDeadline()
		{
			bool order_fix = false;

			for (unsigned int index = 0; index < commandQueue_RT.size(); index++)
			{
				for (unsigned int num = 0; num < commandQueue_RT[index]->getRequestorIndex(); num++) // For all requestors from "num". getRequestorIndex() gives the number of requestors
				{
					if (commandQueue_RT[index]->getRequestorSize(num, true) > 0) // Return the buffer size of the requestor
					{
						if (deadline_set_flag[num] == false)
						{						
							if (clock != 1)
							{
								if (commandQueue_RT[index]->getRequestorCommand(num, true)->busPacketType == PRE)
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
							else
							{
								service_deadline[num] = init_deadline;
							}

							deadline_track[num]    = clock;
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
		

		/**
			Return weather there exits specific cmd in HPA queue.
			It depends on the implementaion.
			@param reqID The requestor ID.
			@param cmd The command that needs to be checked. 			
			@param mode true means looking for same requestor; false otherwise.
		*/
		bool cmdExistsHPA(BusPacketType cmd, unsigned int reqID, bool mode)
		{
			if(mode) 
			{
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
									if((commandQueue[index]->getRequestorCommand(num, false)->busPacketType == cmd) && 
									   (commandQueue[index]->getRequestorCommand(num, false)->requestorID == reqID)) 
									{
										return true;	
									}
								}
							}
						}
					}
				}
			}
			else 
			{
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
									if((commandQueue[index]->getRequestorCommand(num, false)->busPacketType == cmd) && 
									   (commandQueue[index]->getRequestorCommand(num, false)->requestorID != reqID))  
									{
										return true;	
									}
								}
							}
						}
					}
				}
			}
			return false;
		}						
		
		/**
			Return the estimation of the latency for requestor.
			It depends on the implementaion.
			@param requestor The requestor ID.
			@param cmd The command chosen by controller. 			
		*/
		unsigned int WCLator(unsigned int requestor, BusPacket *cmd)
		{
			/**
				Based on DDR3 1600H device timing constraints.
				This table is calculated based on the LPRE and LACT interference.
			*/			
			unsigned int LACT[16] = {9, 15, 21, 27, 34, 40, 46, 52, 59, 65, 71, 77, 84, 90, 96, 102};
			unsigned int LPRE[16] = {2, 3, 6, 7, 9, 11, 13, 14, 17, 18, 19, 22, 23, 26, 27, 29};

			/**
				Based on the state of RT queues, we can extract the following.
			*/
			front_has_pre = 0;
			front_has_act = 0;
			front_has_rd = 0;
			front_has_wr = 0;

			OrderTemp = Order;
			N = 0;

			/**
				Number of requestors that are higher priority than "requestor".
			*/
			for (unsigned int i = 0; i < Order.size(); i++) 
			{
				if (Order.at(i) == requestor)
					N = i;
			}

			/**
				Estimation based on specific implementation decision.
			*/
			if (impl == "C")
			{
				if (cmd != NULL) 
				{
					if (cmd->busPacketType == RD || cmd->busPacketType == WR)
					{
						for (unsigned int h = 0; h < OrderTemp.size(); h++)
						{
							if (OrderTemp.at(h) == cmd->requestorID)
							{
								OrderTemp.erase(OrderTemp.begin() + h);
								OrderTemp.push_back(cmd->requestorID);
								break;
							}
						}
						for (unsigned int i = 0; i < OrderTemp.size(); i++)
						{
							if (OrderTemp.at(i) == requestor)
								N = i; 
						}
					}
				}
			}

			/**
				Number of requestors that are higher priority than "requestor" and has PRE.
			*/
			for (unsigned int traverse = 0; traverse < N; traverse++) 
			{
				oldestTemp = NULL;
				oldestTemp = returnOldest(OrderTemp.at(traverse));
				if (oldestTemp != NULL)
				{
					if (oldestTemp->busPacketType == PRE)
						front_has_pre++;
				}
			}

			/**
				Number of requestors that are higher priority than "requestor" and has ACT.
			*/
			for (unsigned int traverse = 0; traverse < N; traverse++) 
			{
				oldestTemp = NULL;
				oldestTemp = returnOldest(OrderTemp.at(traverse));
				if (oldestTemp != NULL)
				{
					/**
						Number of requestors that are higher priority than "requestor" and has PRE still required to issue ACT; hence, we consider them.
					*/
					if (oldestTemp->busPacketType == ACT)
						front_has_act++;
					else if (oldestTemp->busPacketType == PRE)
					{
						if (commandQueue_RT[oldestTemp->bank]->getRequestorCommand_position(oldestTemp->requestorID, 1, true)->busPacketType == ACT)
							front_has_act++;
					}
				}
			}

			/**
				Number of requestors that are higher priority than "requestor" and has RD.
			*/
			for (unsigned int traverse = 0; traverse < N; traverse++) 
			{
				oldestTemp = NULL;
				oldestTemp = returnOldest(OrderTemp.at(traverse));
				if (oldestTemp != NULL)
				{
					/**
						Number of requestors that are higher priority than "requestor" and has PRE/ACT still required to issue RD; hence, we consider them.
					*/
					if (oldestTemp->busPacketType == RD)
						front_has_rd++;
					else if (oldestTemp->busPacketType == ACT)
					{
						if (commandQueue_RT[oldestTemp->bank]->getRequestorCommand_position(oldestTemp->requestorID, 1, true)->busPacketType == RD)
							front_has_rd++;
					}
					else if (oldestTemp->busPacketType == PRE)
					{
						if (commandQueue_RT[oldestTemp->bank]->getRequestorCommand_position(oldestTemp->requestorID, 2, true)->busPacketType == RD)
							front_has_rd++;
					}
				}
			}

			/**
				Number of requestors that are higher priority than "requestor" and has WR.
			*/
			for (unsigned int traverse = 0; traverse < N; traverse++) // number of requestors that are higher priority than "requestor" and has WR
			{
				oldestTemp = NULL;
				oldestTemp = returnOldest(OrderTemp.at(traverse));
				if (oldestTemp != NULL)
				{
					/**
						Number of requestors that are higher priority than "requestor" and has PRE/ACT still required to issue ACT; hence, we consider them.
					*/
					if (oldestTemp->busPacketType == WR)
						front_has_wr++;
					else if (oldestTemp->busPacketType == ACT)
					{
						if (commandQueue_RT[oldestTemp->bank]->getRequestorCommand_position(oldestTemp->requestorID, 1, true)->busPacketType == WR)
							front_has_wr++;
					}
					else if (oldestTemp->busPacketType == PRE)
					{
						if (commandQueue_RT[oldestTemp->bank]->getRequestorCommand_position(oldestTemp->requestorID, 2, true)->busPacketType == WR)
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

			/**
				Estimation based on specific implementation decision.
			*/
			if 		(impl == "A")
			{
				/**
					Considering the worst-case assuming that HPA command is not known.
				*/
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
									if (CInterR == 0)
										temp_timer[requestor] = 1 + front_has_rd * tCCD + tRL + tBUS;
									else
										temp_timer[requestor] = CInterR + front_has_rd * tCCD + tRL + tBUS;
								}
								else
								{
									temp_timer[requestor] = isReadyTimer(returnOldest(requestor), requestor) + (front_has_rd - 2) * tCCD + max(tRTW, 2 * tCCD) + tWtoR - 1 + tRL + tBUS;
								}
								tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor] : tempLatency;
								temp_timer[requestor] = 0;

								if (isReadyTimer(returnOldest(requestor), requestor) <= 1)
								{
									if (CInterR == 0)
										temp_timer[requestor] = 1 + front_has_rd * tCCD + tRL + tBUS;
									else
										temp_timer[requestor] = CInterR + front_has_rd * tCCD + tRL + tBUS;
								}
								else
								{
									temp_timer[requestor] = isReadyTimer(returnOldest(requestor), requestor) + (front_has_rd - 2) * tCCD + max(tRTW, 2 * tCCD) + tWtoR - 1 + tRL + tBUS;
								}
								tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor] : tempLatency;
								temp_timer[requestor] = 0;

								if (isReadyTimer(returnOldest(requestor), requestor) <= 1)
								{
									if (CInterR == 0)
										temp_timer[requestor] = 1 + front_has_rd * tCCD + tRL + tBUS;
									else
										temp_timer[requestor] = CInterR + front_has_rd * tCCD + tRL + tBUS;
								}
								else
								{
									temp_timer[requestor] = isReadyTimer(returnOldest(requestor), requestor) + (front_has_rd - 2) * tCCD + max(tRTW, 2 * tCCD) + tWtoR - 1 + tRL + tBUS;
								}

								tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor] : tempLatency;
								temp_timer[requestor] = 0;

								if (isReadyTimer(returnOldest(requestor), requestor) <= 1)
								{
									temp_timer[requestor] = tCCD + front_has_rd * tCCD + tRL + tBUS;
								}
								else
								{
									temp_timer[requestor] = isReadyTimer(returnOldest(requestor), requestor) + (front_has_rd - 2) * tCCD + max(tRTW, 2 * tCCD) + tWtoR - 1 + tRL + tBUS;
								}
								tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor] : tempLatency;
								temp_timer[requestor] = 0;

								if (isReadyTimer(returnOldest(requestor), requestor) <= 1)
								{
									temp_timer[requestor] = tWtoR + front_has_rd * tCCD + tRL + tBUS;
								}
								else
								{
									temp_timer[requestor] = isReadyTimer(returnOldest(requestor), requestor) + (front_has_rd - 2) * tCCD + max(tRTW, 2 * tCCD) + tWtoR - 1 + tRL + tBUS;
								}
								tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor] : tempLatency;
								temp_timer[requestor] = 0;

								temp_timer[requestor] = tRL + tBUS;
								tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor] : tempLatency;
								temp_timer[requestor] = 0;
							}
							else if (returnOldest(requestor)->busPacketType == WR)
							{
								if (isReadyTimer(returnOldest(requestor), requestor) <= 1)
								{
									if (CInterW == 0)
										temp_timer[requestor] = 1 + (REQ_count - 1) * tCCD + tRTW + tWL + tBUS;
									else
										temp_timer[requestor] = CInterW + (REQ_count - 1) * tCCD + tRTW + tWL + tBUS;
								}
								else
								{
									temp_timer[requestor] = isReadyTimer(returnOldest(requestor), requestor) + (REQ_count - 1) * tCCD + tRTW + 1 + tWL + tBUS;
								}
								tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor] : tempLatency;
								temp_timer[requestor] = 0;

								if (isReadyTimer(returnOldest(requestor), requestor) <= 1)
								{
									if (CInterW == 0)
										temp_timer[requestor] = 1 + (REQ_count - 1) * tCCD + tRTW + tWL + tBUS;
									else
										temp_timer[requestor] = CInterW + (REQ_count - 1) * tCCD + tRTW + tWL + tBUS;
								}
								else
								{
									temp_timer[requestor] = isReadyTimer(returnOldest(requestor), requestor) + (REQ_count - 1) * tCCD + tRTW + 1 + tWL + tBUS;
								}
								tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor] : tempLatency;
								temp_timer[requestor] = 0;

								if (isReadyTimer(returnOldest(requestor), requestor) <= 1)
								{
									if (CInterW == 0)
										temp_timer[requestor] = 1 + (REQ_count - 1) * tCCD + tRTW + tWL + tBUS;
									else
										temp_timer[requestor] = CInterW + (REQ_count - 1) * tCCD + tRTW + tWL + tBUS;
								}
								else
								{
									temp_timer[requestor] = isReadyTimer(returnOldest(requestor), requestor) + (REQ_count - 1) * tCCD + tRTW + 1 + tWL + tBUS;
								}
								tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor] : tempLatency;
								temp_timer[requestor] = 0;

								if (isReadyTimer(returnOldest(requestor), requestor) <= 1)
								{
									temp_timer[requestor] = tRTW + (REQ_count - 1) * tCCD + tRTW + tWL + tBUS;
								}
								else
								{
									temp_timer[requestor] = isReadyTimer(returnOldest(requestor), requestor) + (REQ_count - 1) * tCCD + tRTW + 1 + tWL + tBUS;
								}
								tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor] : tempLatency;
								temp_timer[requestor] = 0;

								if (isReadyTimer(returnOldest(requestor), requestor) <= 1)
								{
									temp_timer[requestor] = tCCD + (REQ_count - 1) * tCCD + tRTW + tWL + tBUS;
								}
								else
								{
									temp_timer[requestor] = isReadyTimer(returnOldest(requestor), requestor) + (REQ_count - 1) * tCCD + tRTW + 1 + tWL + tBUS;
								}
								tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor] : tempLatency;
								temp_timer[requestor] = 0;

								temp_timer[requestor] = tWL + tBUS;
								tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor] : tempLatency;
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
							tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor] : tempLatency;
							temp_timer[requestor] = 0;

							temp_timer[requestor] = max(isReadyTimer(returnOldest(requestor), requestor), tWR) +
													LPRE[front_has_pre] + tRP + LACT[front_has_act] + tRCD + (front_has_rd - 2) * tCCD + max(tRTW, 2 * tCCD) + tWtoR - 1 + tRL + tBUS;
							tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor] : tempLatency;
							temp_timer[requestor] = 0;

							temp_timer[requestor] = tRP + LACT[front_has_act] + tRCD + (front_has_rd - 2) * tCCD + max(tRTW, 2 * tCCD) + tWtoR - 1 + tRL + tBUS;
							tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor] : tempLatency;
							temp_timer[requestor] = 0;

							temp_timer[requestor] = isReadyTimer(returnOldest(requestor), requestor) + LPRE[front_has_pre] + tRP + LACT[front_has_act] + tRCD + (front_has_rd - 2) * tCCD + max(tRTW, 2 * tCCD) + tWtoR - 1 + tRL + tBUS;
							tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor] : tempLatency;
							temp_timer[requestor] = 0;

							if (isReadyTimer(returnOldest(requestor), requestor) > 0)
								temp_timer[requestor] = isReadyTimer(returnOldest(requestor), requestor) + LPRE[front_has_pre] + tRP + LACT[front_has_act] + tRCD + (front_has_rd - 2) * tCCD + max(tRTW, 2 * tCCD) + tWtoR - 1 + tRL + tBUS;
							else if (isReadyTimer(returnOldest(requestor), requestor) == 0)
								temp_timer[requestor] = isReadyTimer(returnOldest(requestor), requestor) + LPRE[front_has_pre] + tRP + LACT[front_has_act] + tRCD + (front_has_rd - 2) * tCCD + max(tRTW, 2 * tCCD) + tWtoR - 1 + 1 + tRL + tBUS;
							tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor] : tempLatency;
							temp_timer[requestor] = 0;
						}
						else if (returnOldest(requestor)->busPacketType == ACT)
						{

							temp_timer[requestor] = isReadyTimer(returnOldest(requestor), requestor) + LACT[front_has_act] + tRCD + (front_has_rd - 2) * tCCD + max(tRTW, 2 * tCCD) + tWtoR - 1 + tRL + tBUS;
							tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor] : tempLatency;
							temp_timer[requestor] = 0;

							temp_timer[requestor] = tRCD + (front_has_rd - 2) * tCCD + max(tRTW, 2 * tCCD) + tWtoR - 1 + tRL + tBUS;
							tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor] : tempLatency;
							temp_timer[requestor] = 0;

							temp_timer[requestor] = isReadyTimer(returnOldest(requestor), requestor) + LACT[front_has_act] + tRCD + (front_has_rd - 2) * tCCD + max(tRTW, 2 * tCCD) + tWtoR - 1 + tRL + tBUS;
							tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor] : tempLatency;
							temp_timer[requestor] = 0;
						}
						else if (returnOldest(requestor)->busPacketType == RD)
						{
							if (isReadyTimer(returnOldest(requestor), requestor) <= 1)
							{
								if (CInterR == 0)
									temp_timer[requestor] = 1 + front_has_rd * tCCD + tRL + tBUS;
								else
									temp_timer[requestor] = CInterR + front_has_rd * tCCD + tRL + tBUS;

								tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor] : tempLatency;
								temp_timer[requestor] = 0;
							}
							else
							{
								temp_timer[requestor] = isReadyTimer(returnOldest(requestor), requestor) + (front_has_rd - 2) * tCCD + max(tRTW, 2 * tCCD) + tWtoR - 1 + tRL + tBUS;
								tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor] : tempLatency;
								temp_timer[requestor] = 0;
							}
							if (isReadyTimer(returnOldest(requestor), requestor) <= 1)
							{
								temp_timer[requestor] = tCCD + front_has_rd * tCCD + tRL + tBUS;
								tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor] : tempLatency;
								temp_timer[requestor] = 0;
							}
							else
							{
								temp_timer[requestor] = isReadyTimer(returnOldest(requestor), requestor) + (front_has_rd - 2) * tCCD + max(tRTW, 2 * tCCD) + tWtoR - 1 + tRL + tBUS;
								tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor] : tempLatency;
								temp_timer[requestor] = 0;
							}
							if (isReadyTimer(returnOldest(requestor), requestor) <= 1)
							{
								temp_timer[requestor] = tWtoR + front_has_rd * tCCD + tRL + tBUS;
								tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor] : tempLatency;
								temp_timer[requestor] = 0;
							}
							else
							{
								temp_timer[requestor] = isReadyTimer(returnOldest(requestor), requestor) + (front_has_rd - 2) * tCCD + max(tRTW, 2 * tCCD) + tWtoR - 1 + tRL + tBUS;
								tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor] : tempLatency;
								temp_timer[requestor] = 0;
							}
							temp_timer[requestor] = tRL + tBUS;
							tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor] : tempLatency;
							temp_timer[requestor] = 0;
						}
						else if (returnOldest(requestor)->busPacketType == WR)
						{
							if (isReadyTimer(returnOldest(requestor), requestor) <= 1)
							{
								if (CInterW == 0)
									temp_timer[requestor] = 1 + (REQ_count - 1) * tCCD + tRTW + tWL + tBUS;
								else
									temp_timer[requestor] = CInterW + (REQ_count - 1) * tCCD + tRTW + tWL + tBUS;

								tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor] : tempLatency;
								temp_timer[requestor] = 0;
							}
							else
							{
								temp_timer[requestor] = isReadyTimer(returnOldest(requestor), requestor) + (REQ_count - 1) * tCCD + tRTW + 1 + tWL + tBUS;
								tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor] : tempLatency;
								temp_timer[requestor] = 0;
							}
							if (isReadyTimer(returnOldest(requestor), requestor) <= 1)
							{
								if (CInterW == 0)
									temp_timer[requestor] = 1 + (REQ_count - 1) * tCCD + tRTW + tWL + tBUS;
								else
									temp_timer[requestor] = CInterW + (REQ_count - 1) * tCCD + tRTW + tWL + tBUS;

								tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor] : tempLatency;
								temp_timer[requestor] = 0;
							}
							else
							{
								temp_timer[requestor] = isReadyTimer(returnOldest(requestor), requestor) + (REQ_count - 1) * tCCD + tRTW + 1 + tWL + tBUS;
								tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor] : tempLatency;
								temp_timer[requestor] = 0;
							}
							if (isReadyTimer(returnOldest(requestor), requestor) <= 1)
							{
								if (CInterW == 0)
									temp_timer[requestor] = 1 + (REQ_count - 1) * tCCD + tRTW + tWL + tBUS;
								else
									temp_timer[requestor] = CInterW + (REQ_count - 1) * tCCD + tRTW + tWL + tBUS;

								tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor] : tempLatency;
								temp_timer[requestor] = 0;
							}
							else
							{
								temp_timer[requestor] = isReadyTimer(returnOldest(requestor), requestor) + (REQ_count - 1) * tCCD + tRTW + 1 + tWL + tBUS;
								tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor] : tempLatency;
								temp_timer[requestor] = 0;
							}
							if (isReadyTimer(returnOldest(requestor), requestor) <= 1)
							{
								temp_timer[requestor] = tRTW + front_has_rd * tCCD + (REQ_count - 1) * tCCD + tRTW + tWL + tBUS;
								tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor] : tempLatency;
								temp_timer[requestor] = 0;
							}
							else
							{
								temp_timer[requestor] = isReadyTimer(returnOldest(requestor), requestor) + (REQ_count - 1) * tCCD + tRTW + 1 + tWL + tBUS;
								tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor] : tempLatency;
								temp_timer[requestor] = 0;
							}
							if (isReadyTimer(returnOldest(requestor), requestor) <= 1)
							{
								temp_timer[requestor] = tCCD + (REQ_count - 1) * tCCD + tRTW + tWL + tBUS;
								tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor] : tempLatency;
								temp_timer[requestor] = 0;
							}
							else
							{
								temp_timer[requestor] = isReadyTimer(returnOldest(requestor), requestor) + (REQ_count - 1) * tCCD + tRTW + 1 + tWL + tBUS;
								tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor] : tempLatency;
								temp_timer[requestor] = 0;
							}
							temp_timer[requestor] = tWL + tBUS;
							tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor] : tempLatency;
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
								if (CInterR == 0)
									temp_timer[requestor] = 1 + front_has_rd * tCCD + tRL + tBUS;
								else
									temp_timer[requestor] = CInterR + front_has_rd * tCCD + tRL + tBUS;

								tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor] : tempLatency;
								temp_timer[requestor] = 0;
							}
							else
							{
								temp_timer[requestor] = isReadyTimer(returnOldest(requestor), requestor) + (front_has_rd - 2) * tCCD + max(tRTW, 2 * tCCD) + tWtoR - 1 + tRL + tBUS;
								tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor] : tempLatency;
								temp_timer[requestor] = 0;
							}

							if (isReadyTimer(returnOldest(requestor), requestor) <= 1)
							{
								temp_timer[requestor] = tCCD + front_has_rd * tCCD + tRL + tBUS;
								tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor] : tempLatency;
								temp_timer[requestor] = 0;
							}
							else
							{
								temp_timer[requestor] = isReadyTimer(returnOldest(requestor), requestor) + (front_has_rd - 2) * tCCD + max(tRTW, 2 * tCCD) + tWtoR - 1 + tRL + tBUS;
								tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor] : tempLatency;
								temp_timer[requestor] = 0;
							}
							if (isReadyTimer(returnOldest(requestor), requestor) <= 1)
							{
								temp_timer[requestor] = tWtoR + front_has_rd * tCCD + tRL + tBUS;
								tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor] : tempLatency;
								temp_timer[requestor] = 0;
							}
							else
							{
								temp_timer[requestor] = isReadyTimer(returnOldest(requestor), requestor) + (front_has_rd - 2) * tCCD + max(tRTW, 2 * tCCD) + tWtoR - 1 + tRL + tBUS;
								tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor] : tempLatency;
								temp_timer[requestor] = 0;
							}

							temp_timer[requestor] = tRL + tBUS;
							tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor] : tempLatency;
							temp_timer[requestor] = 0;
						}
						else if (returnOldest(requestor)->busPacketType == WR)
						{
							req_open[requestor] = true;							
							if (isReadyTimer(returnOldest(requestor), requestor) <= 1)
							{
								if (CInterW == 0)
									temp_timer[requestor] = 1 + (REQ_count - 1) * tCCD + tRTW + tWL + tBUS;
								else
									temp_timer[requestor] = CInterW + (REQ_count - 1) * tCCD + tRTW + tWL + tBUS;

								tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor] : tempLatency;
								temp_timer[requestor] = 0;
							}
							else
							{
								temp_timer[requestor] = isReadyTimer(returnOldest(requestor), requestor) + (REQ_count - 1) * tCCD + tRTW + 1 + tWL + tBUS;
								tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor] : tempLatency;
								temp_timer[requestor] = 0;
							}
							if (isReadyTimer(returnOldest(requestor), requestor) <= 1)
							{
								temp_timer[requestor] = tRTW + (REQ_count - 1) * tCCD + tRTW + tWL + tBUS;
								tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor] : tempLatency;
								temp_timer[requestor] = 0;
							}
							else
							{
								temp_timer[requestor] = isReadyTimer(returnOldest(requestor), requestor) + (REQ_count - 1) * tCCD + tRTW + 1 + tWL + tBUS;
								tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor] : tempLatency;
								temp_timer[requestor] = 0;
							}
							if (isReadyTimer(returnOldest(requestor), requestor) <= 1)
							{
								temp_timer[requestor] = tCCD + (REQ_count - 1) * tCCD + tRTW + tWL + tBUS;
								tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor] : tempLatency;
								temp_timer[requestor] = 0;
							}
							else
							{
								temp_timer[requestor] = isReadyTimer(returnOldest(requestor), requestor) + (REQ_count - 1) * tCCD + tRTW + 1 + tWL + tBUS;
								tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor] : tempLatency;
								temp_timer[requestor] = 0;
							}
							temp_timer[requestor] = tWL + tBUS;
							tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor] : tempLatency;
							temp_timer[requestor] = 0;							
						}
						else if (returnOldest(requestor)->busPacketType == PRE) // done modifications
						{
							req_open[requestor] = false;

							temp_timer[requestor] = max(isReadyTimer(returnOldest(requestor), requestor), tRTP) +
													LPRE[front_has_pre] + tRP + LACT[front_has_act] + tRCD + (front_has_rd - 2) * tCCD + max(tRTW, 2 * tCCD) + tWtoR - 1 + tRL + tBUS;
							tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor] : tempLatency;
							temp_timer[requestor] = 0;

							temp_timer[requestor] = max(isReadyTimer(returnOldest(requestor), requestor), tWR) + LPRE[front_has_pre] + tRP + LACT[front_has_act] + tRCD + (front_has_rd - 2) * tCCD + max(tRTW, 2 * tCCD) + tWtoR - 1 + tRL + tBUS;
							tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor] : tempLatency;
							temp_timer[requestor] = 0;

							temp_timer[requestor] = tRP + LACT[front_has_act] + tRCD + (front_has_rd - 2) * tCCD + max(tRTW, 2 * tCCD) + tWtoR - 1 + tRL + tBUS;
							tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor] : tempLatency;
							temp_timer[requestor] = 0;

							temp_timer[requestor] = isReadyTimer(returnOldest(requestor), requestor) + LPRE[front_has_pre] + tRP + LACT[front_has_act] + tRCD + (front_has_rd - 2) * tCCD + max(tRTW, 2 * tCCD) + tWtoR - 1 + tRL + tBUS;
							tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor] : tempLatency;
							temp_timer[requestor] = 0;

							if (isReadyTimer(returnOldest(requestor), requestor) > 0)
								temp_timer[requestor] = isReadyTimer(returnOldest(requestor), requestor) + LPRE[front_has_pre] + tRP + LACT[front_has_act] + tRCD + (front_has_rd - 2) * tCCD + max(tRTW, 2 * tCCD) + tWtoR - 1 + tRL + tBUS;
							else if (isReadyTimer(returnOldest(requestor), requestor) == 0)
								temp_timer[requestor] = isReadyTimer(returnOldest(requestor), requestor) + LPRE[front_has_pre] + tRP + LACT[front_has_act] + tRCD + (front_has_rd - 2) * tCCD + max(tRTW, 2 * tCCD) + tWtoR - 1 + 1 + tRL + tBUS;

							tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor] : tempLatency;
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
			else if (impl == "B")
			{
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
									if (CInterR == 0)
										temp_timer[requestor] = 1 + front_has_rd * tCCD + tRL + tBUS;
									else
										temp_timer[requestor] = CInterR + front_has_rd * tCCD + tRL + tBUS;
								}
								else
								{
									temp_timer[requestor] = isReadyTimer(returnOldest(requestor), requestor) + (front_has_rd - 2) * tCCD + max(tRTW, 2 * tCCD) + tWtoR - 1 + tRL + tBUS;
								}
								tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor] : tempLatency;
								temp_timer[requestor] = 0;								
								if(cmdExistsHPA(PRE, requestor, false))
								{
									if (isReadyTimer(returnOldest(requestor), requestor) <= 1)
									{
										if (CInterR == 0)
											temp_timer[requestor] = 1 + front_has_rd * tCCD + tRL + tBUS;
										else
											temp_timer[requestor] = CInterR + front_has_rd * tCCD + tRL + tBUS;
									}
									else
									{
										temp_timer[requestor] = isReadyTimer(returnOldest(requestor), requestor) + (front_has_rd - 2) * tCCD + max(tRTW, 2 * tCCD) + tWtoR - 1 + tRL + tBUS;
									}
									tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor] : tempLatency;
									temp_timer[requestor] = 0;
								}
								if(cmdExistsHPA(PRE, requestor, false))
								{
									if (isReadyTimer(returnOldest(requestor), requestor) <= 1)
									{
										if (CInterR == 0)
											temp_timer[requestor] = 1 + front_has_rd * tCCD + tRL + tBUS;
										else
											temp_timer[requestor] = CInterR + front_has_rd * tCCD + tRL + tBUS;
									}
									else
									{
										temp_timer[requestor] = isReadyTimer(returnOldest(requestor), requestor) + (front_has_rd - 2) * tCCD + max(tRTW, 2 * tCCD) + tWtoR - 1 + tRL + tBUS;
									}
									tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor] : tempLatency;
									temp_timer[requestor] = 0;
								}
								if(cmdExistsHPA(RD, requestor, false))
								{
									if (isReadyTimer(returnOldest(requestor), requestor) <= 1)
									{
										temp_timer[requestor] = tCCD + front_has_rd * tCCD + tRL + tBUS;
									}
									else
									{
										temp_timer[requestor] = isReadyTimer(returnOldest(requestor), requestor) + (front_has_rd - 2) * tCCD + max(tRTW, 2 * tCCD) + tWtoR - 1 + tRL + tBUS;
									}
									tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor] : tempLatency;
									temp_timer[requestor] = 0;
								}
								if(cmdExistsHPA(WR, requestor, false))
								{
									if (isReadyTimer(returnOldest(requestor), requestor) <= 1)
									{
										temp_timer[requestor] = tWtoR + front_has_rd * tCCD + tRL + tBUS;
									}
									else
									{
										temp_timer[requestor] = isReadyTimer(returnOldest(requestor), requestor) + (front_has_rd - 2) * tCCD + max(tRTW, 2 * tCCD) + tWtoR - 1 + tRL + tBUS;
									}	
									tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor] : tempLatency;
									temp_timer[requestor] = 0;
								}
								if(cmdExistsHPA(RD, requestor, true))
								{
									temp_timer[requestor] = tRL + tBUS;
									tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor] : tempLatency;
									temp_timer[requestor] = 0;
								}
							}
							else if (returnOldest(requestor)->busPacketType == WR)
							{
								if (isReadyTimer(returnOldest(requestor), requestor) <= 1)
								{
									if (CInterW == 0)
										temp_timer[requestor] = 1 + (REQ_count - 1) * tCCD + tRTW + tWL + tBUS;
									else
										temp_timer[requestor] = CInterW + (REQ_count - 1) * tCCD + tRTW + tWL + tBUS;
								}
								else
								{
									temp_timer[requestor] = isReadyTimer(returnOldest(requestor), requestor) + (REQ_count - 1) * tCCD + tRTW + 1 + tWL + tBUS;
								}
								tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor] : tempLatency;
								temp_timer[requestor] = 0;								
								if(cmdExistsHPA(PRE, requestor, false))
								{
									if (isReadyTimer(returnOldest(requestor), requestor) <= 1)
									{
										if (CInterW == 0)
											temp_timer[requestor] = 1 + (REQ_count - 1) * tCCD + tRTW + tWL + tBUS;
										else
											temp_timer[requestor] = CInterW + (REQ_count - 1) * tCCD + tRTW + tWL + tBUS;
									}
									else
									{
										temp_timer[requestor] = isReadyTimer(returnOldest(requestor), requestor) + (REQ_count - 1) * tCCD + tRTW + 1 + tWL + tBUS;
									}
									tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor] : tempLatency;
									temp_timer[requestor] = 0;
								}
								if(cmdExistsHPA(ACT, requestor, false))
								{
									if (isReadyTimer(returnOldest(requestor), requestor) <= 1)
									{
										if (CInterW == 0)
											temp_timer[requestor] = 1 + (REQ_count - 1) * tCCD + tRTW + tWL + tBUS;
										else
											temp_timer[requestor] = CInterW + (REQ_count - 1) * tCCD + tRTW + tWL + tBUS;
									}
									else
									{
										temp_timer[requestor] = isReadyTimer(returnOldest(requestor), requestor) + (REQ_count - 1) * tCCD + tRTW + 1 + tWL + tBUS;
									}
									tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor] : tempLatency;
									temp_timer[requestor] = 0;
								}
								if(cmdExistsHPA(RD, requestor, false))
								{
									if (isReadyTimer(returnOldest(requestor), requestor) <= 1)
									{
										temp_timer[requestor] = tRTW + (REQ_count - 1) * tCCD + tRTW + tWL + tBUS;
									}
									else
									{
										temp_timer[requestor] = isReadyTimer(returnOldest(requestor), requestor) + (REQ_count - 1) * tCCD + tRTW + 1 + tWL + tBUS;
									}
									tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor] : tempLatency;
									temp_timer[requestor] = 0;
								}
								if(cmdExistsHPA(WR, requestor, false))
								{
									if (isReadyTimer(returnOldest(requestor), requestor) <= 1)
									{
										temp_timer[requestor] = tCCD + (REQ_count - 1) * tCCD + tRTW + tWL + tBUS;
									}
									else
									{
										temp_timer[requestor] = isReadyTimer(returnOldest(requestor), requestor) + (REQ_count - 1) * tCCD + tRTW + 1 + tWL + tBUS;
									}
									tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor] : tempLatency;
									temp_timer[requestor] = 0;
								}
								if(cmdExistsHPA(WR, requestor, true))
								{	
									temp_timer[requestor] = tWL + tBUS;
									tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor] : tempLatency;
									temp_timer[requestor] = 0;
								}
							}
						}
					}
					else if (req_open[requestor] == false)
					{
						if (returnOldest(requestor)->busPacketType == PRE)
						{
							req_open[requestor] = false;

							// HPA can always send a NULL request
							if (isReadyTimer(returnOldest(requestor), requestor) > 0)
								temp_timer[requestor] = isReadyTimer(returnOldest(requestor), requestor) + LPRE[front_has_pre] + tRP + LACT[front_has_act] + tRCD + (front_has_rd - 2) * tCCD + max(tRTW, 2 * tCCD) + tWtoR - 1 + tRL + tBUS;
							else if (isReadyTimer(returnOldest(requestor), requestor) == 0)
								temp_timer[requestor] = isReadyTimer(returnOldest(requestor), requestor) + LPRE[front_has_pre] + tRP + LACT[front_has_act] + tRCD + (front_has_rd - 2) * tCCD + max(tRTW, 2 * tCCD) + tWtoR - 1 + 1 + tRL + tBUS;

							tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor] : tempLatency;
							temp_timer[requestor] = 0;					

							if(cmdExistsHPA(RD, requestor, true)) 
							{
								temp_timer[requestor] = max(isReadyTimer(returnOldest(requestor), requestor), tRTP) +
									LPRE[front_has_pre] + tRP + LACT[front_has_act] + tRCD + (front_has_rd - 2) * tCCD + max(tRTW, 2 * tCCD) + tWtoR - 1 + tRL + tBUS;
								tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor] : tempLatency;
								temp_timer[requestor] = 0;			
							}
							if(cmdExistsHPA(WR, requestor, true)) 
							{
								temp_timer[requestor] = max(isReadyTimer(returnOldest(requestor), requestor), tWR) +
									LPRE[front_has_pre] + tRP + LACT[front_has_act] + tRCD + (front_has_rd - 2) * tCCD + max(tRTW, 2 * tCCD) + tWtoR - 1 + tRL + tBUS;
								tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor] : tempLatency;
								temp_timer[requestor] = 0;		
							}
							if(cmdExistsHPA(ACT, requestor, true) || cmdExistsHPA(PRE, requestor, true)) 
							{
								temp_timer[requestor] = tRP + LACT[front_has_act] + tRCD + (front_has_rd - 2) * tCCD + max(tRTW, 2 * tCCD) + tWtoR - 1 + tRL + tBUS;
								tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor] : tempLatency;
								temp_timer[requestor] = 0;	

							}
							if(cmdExistsHPA(RD, requestor, false) || cmdExistsHPA(WR, requestor, false) || cmdExistsHPA(ACT, requestor, false) || cmdExistsHPA(PRE, requestor, false)) 
							{
								temp_timer[requestor] = isReadyTimer(returnOldest(requestor), requestor) + LPRE[front_has_pre] + tRP + LACT[front_has_act] + tRCD + (front_has_rd - 2) * tCCD + max(tRTW, 2 * tCCD) + tWtoR - 1 + tRL + tBUS;
								tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor] : tempLatency;
								temp_timer[requestor] = 0;	
							}

						}
						else if (returnOldest(requestor)->busPacketType == ACT)
						{
							// HPA can always send a NULL request
							temp_timer[requestor] = isReadyTimer(returnOldest(requestor), requestor) + LACT[front_has_act] + tRCD + (front_has_rd - 2) * tCCD + max(tRTW, 2 * tCCD) + tWtoR - 1 + tRL + tBUS;
							tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor] : tempLatency;
							temp_timer[requestor] = 0;	

							if(cmdExistsHPA(RD, requestor, false) || cmdExistsHPA(WR, requestor, false) || cmdExistsHPA(ACT, requestor, false) || cmdExistsHPA(PRE, requestor, false)) 
							{
								temp_timer[requestor] = isReadyTimer(returnOldest(requestor), requestor) + LACT[front_has_act] + tRCD + (front_has_rd - 2) * tCCD + max(tRTW, 2 * tCCD) + tWtoR - 1 + tRL + tBUS;
								tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor] : tempLatency;
								temp_timer[requestor] = 0;	
							}
							if(cmdExistsHPA(RD, requestor, true) || cmdExistsHPA(WR, requestor, true) || cmdExistsHPA(ACT, requestor, true) || cmdExistsHPA(PRE, requestor, true)) 
							{
								temp_timer[requestor] = tRCD + (front_has_rd - 2) * tCCD + max(tRTW, 2 * tCCD) + tWtoR - 1 + tRL + tBUS;
								tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor] : tempLatency;
								temp_timer[requestor] = 0;	
							}
						}
						else if (returnOldest(requestor)->busPacketType == RD)
						{
							// HPA can always send a NULL request
							if (isReadyTimer(returnOldest(requestor), requestor) <= 1)
							{
								if (CInterR == 0)
									temp_timer[requestor] = 1 + front_has_rd * tCCD + tRL + tBUS;
								else
									temp_timer[requestor] = CInterR + front_has_rd * tCCD + tRL + tBUS;
							}
							else
							{
								temp_timer[requestor] = isReadyTimer(returnOldest(requestor), requestor) + (front_has_rd - 2) * tCCD + max(tRTW, 2 * tCCD) + tWtoR - 1 + tRL + tBUS;
							}
							tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor] : tempLatency;
							temp_timer[requestor] = 0;	
							if(cmdExistsHPA(PRE, requestor, false))
							{
								if (isReadyTimer(returnOldest(requestor), requestor) <= 1)
								{
									if (CInterR == 0)
										temp_timer[requestor] = 1 + front_has_rd * tCCD + tRL + tBUS;
									else
										temp_timer[requestor] = CInterR + front_has_rd * tCCD + tRL + tBUS;									
								}
								else
								{
									temp_timer[requestor] = isReadyTimer(returnOldest(requestor), requestor) + (front_has_rd - 2) * tCCD + max(tRTW, 2 * tCCD) + tWtoR - 1 + tRL + tBUS;
								}
								tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor] : tempLatency;
								temp_timer[requestor] = 0;	
							} 
							if(cmdExistsHPA(ACT, requestor, false))
							{
								if (isReadyTimer(returnOldest(requestor), requestor) <= 1)
								{
									if (CInterR == 0)
										temp_timer[requestor] = 1 + front_has_rd * tCCD + tRL + tBUS;
									else
										temp_timer[requestor] = CInterR + front_has_rd * tCCD + tRL + tBUS;
								}
								else
								{
									temp_timer[requestor] = isReadyTimer(returnOldest(requestor), requestor) + (front_has_rd - 2) * tCCD + max(tRTW, 2 * tCCD) + tWtoR - 1 + tRL + tBUS;
								}
								tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor] : tempLatency;
								temp_timer[requestor] = 0;	
							}
							if(cmdExistsHPA(RD, requestor, false))
							{
								if (isReadyTimer(returnOldest(requestor), requestor) <= 1)
								{
									temp_timer[requestor] = tCCD + front_has_rd * tCCD + tRL + tBUS;
								}
								else
								{
									temp_timer[requestor] = isReadyTimer(returnOldest(requestor), requestor) + (front_has_rd - 2) * tCCD + max(tRTW, 2 * tCCD) + tWtoR - 1 + tRL + tBUS;
								}
								tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor] : tempLatency;
								temp_timer[requestor] = 0;	
							}
							if(cmdExistsHPA(WR, requestor, false))
							{
								if (isReadyTimer(returnOldest(requestor), requestor) <= 1)
								{
									temp_timer[requestor] = tWtoR + front_has_rd * tCCD + tRL + tBUS;
								}
								else
								{
									temp_timer[requestor] = isReadyTimer(returnOldest(requestor), requestor) + (front_has_rd - 2) * tCCD + max(tRTW, 2 * tCCD) + tWtoR - 1 + tRL + tBUS;
								}
								tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor] : tempLatency;
								temp_timer[requestor] = 0;
							}
							if(cmdExistsHPA(RD, requestor, true))
							{
								temp_timer[requestor] = tRL + tBUS;
								tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor] : tempLatency;
								temp_timer[requestor] = 0;
							}
						}
						else if (returnOldest(requestor)->busPacketType == WR)
						{
							// HPA can always send a NULL request
							if (isReadyTimer(returnOldest(requestor), requestor) <= 1)
							{
								if (CInterW == 0)
									temp_timer[requestor] = 1 + (REQ_count - 1) * tCCD + tRTW + tWL + tBUS;
								else
									temp_timer[requestor] = CInterW + (REQ_count - 1) * tCCD + tRTW + tWL + tBUS;
							}
							else
							{
								temp_timer[requestor] = isReadyTimer(returnOldest(requestor), requestor) + (REQ_count - 1) * tCCD + tRTW + 1 + tWL + tBUS;
							}
							tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor] : tempLatency;
							temp_timer[requestor] = 0;

							if(cmdExistsHPA(PRE, requestor, false))
							{
								if (isReadyTimer(returnOldest(requestor), requestor) <= 1)
								{
									if (CInterW == 0)
										temp_timer[requestor] = 1 + (REQ_count - 1) * tCCD + tRTW + tWL + tBUS;
									else
										temp_timer[requestor] = CInterW + (REQ_count - 1) * tCCD + tRTW + tWL + tBUS;
								}
								else
								{
									temp_timer[requestor] = isReadyTimer(returnOldest(requestor), requestor) + (REQ_count - 1) * tCCD + tRTW + 1 + tWL + tBUS;
								}
								tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor] : tempLatency;
								temp_timer[requestor] = 0;
							}
							if(cmdExistsHPA(ACT, requestor, false))
							{
								if (isReadyTimer(returnOldest(requestor), requestor) <= 1)
								{
									if (CInterW == 0)
										temp_timer[requestor] = 1 + (REQ_count - 1) * tCCD + tRTW + tWL + tBUS;
									else
										temp_timer[requestor] = CInterW + (REQ_count - 1) * tCCD + tRTW + tWL + tBUS;
								}
								else
								{
									temp_timer[requestor] = isReadyTimer(returnOldest(requestor), requestor) + (REQ_count - 1) * tCCD + tRTW + 1 + tWL + tBUS;
								}
								tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor] : tempLatency;
								temp_timer[requestor] = 0;
							}
							if(cmdExistsHPA(RD, requestor, false))
							{
								if (isReadyTimer(returnOldest(requestor), requestor) <= 1)
								{
									temp_timer[requestor] = tRTW + front_has_rd * tCCD + (REQ_count - 1) * tCCD + tRTW + tWL + tBUS;
								}
								else
								{
									temp_timer[requestor] = isReadyTimer(returnOldest(requestor), requestor) + (REQ_count - 1) * tCCD + tRTW + 1 + tWL + tBUS;
								}
								tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor] : tempLatency;
								temp_timer[requestor] = 0;
							}
							if(cmdExistsHPA(WR, requestor, false))
							{
								if (isReadyTimer(returnOldest(requestor), requestor) <= 1)
								{
									temp_timer[requestor] = tCCD + (REQ_count - 1) * tCCD + tRTW + tWL + tBUS;
								}
								else
								{
									temp_timer[requestor] = isReadyTimer(returnOldest(requestor), requestor) + (REQ_count - 1) * tCCD + tRTW + 1 + tWL + tBUS;
								}
								tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor] : tempLatency;
								temp_timer[requestor] = 0;
							}
							if(cmdExistsHPA(WR, requestor, true))
							{
								temp_timer[requestor] = tWL + tBUS;
								tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor] : tempLatency;
								temp_timer[requestor] = 0;
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

							if (isReadyTimer(returnOldest(requestor), requestor) <= 1)
							{
								if (CInterR == 0)
									temp_timer[requestor] = 1 + front_has_rd * tCCD + tRL + tBUS;
								else
									temp_timer[requestor] = CInterR + front_has_rd * tCCD + tRL + tBUS;
							}
							else
							{
								temp_timer[requestor] = isReadyTimer(returnOldest(requestor), requestor) + (front_has_rd - 2) * tCCD + max(tRTW, 2 * tCCD) + tWtoR - 1 + tRL + tBUS;
							}
							tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor] : tempLatency;
							temp_timer[requestor] = 0;

							if(cmdExistsHPA(PRE, requestor, false))
							{
								if (isReadyTimer(returnOldest(requestor), requestor) <= 1)
								{
									if (CInterR == 0)
										temp_timer[requestor] = 1 + front_has_rd * tCCD + tRL + tBUS;
									else
										temp_timer[requestor] = CInterR + front_has_rd * tCCD + tRL + tBUS;
								}
								else
								{
									temp_timer[requestor] = isReadyTimer(returnOldest(requestor), requestor) + (front_has_rd - 2) * tCCD + max(tRTW, 2 * tCCD) + tWtoR - 1 + tRL + tBUS;
								}
								tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor] : tempLatency;
								temp_timer[requestor] = 0;
							}
							if(cmdExistsHPA(ACT, requestor, false))
							{
								if (isReadyTimer(returnOldest(requestor), requestor) <= 1)
								{
									if (CInterR == 0)
										temp_timer[requestor] = 1 + front_has_rd * tCCD + tRL + tBUS;
									else
										temp_timer[requestor] = CInterR + front_has_rd * tCCD + tRL + tBUS;
								}
								else
								{
									temp_timer[requestor] = isReadyTimer(returnOldest(requestor), requestor) + (front_has_rd - 2) * tCCD + max(tRTW, 2 * tCCD) + tWtoR - 1 + tRL + tBUS;
								}
								tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor] : tempLatency;
								temp_timer[requestor] = 0;
							}
							if(cmdExistsHPA(RD, requestor, false)) 
							{
								if (isReadyTimer(returnOldest(requestor), requestor) <= 1)
								{
									temp_timer[requestor] = tCCD + front_has_rd * tCCD + tRL + tBUS;
								}
								else
								{
									temp_timer[requestor] = isReadyTimer(returnOldest(requestor), requestor) + (front_has_rd - 2) * tCCD + max(tRTW, 2 * tCCD) + tWtoR - 1 + tRL + tBUS;
								}
								tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor] : tempLatency;
								temp_timer[requestor] = 0;
							}	
							if(cmdExistsHPA(WR, requestor, false)) 
							{
								if (isReadyTimer(returnOldest(requestor), requestor) <= 1)
								{
									temp_timer[requestor] = tWtoR + front_has_rd * tCCD + tRL + tBUS;
								}
								else
								{
									temp_timer[requestor] = isReadyTimer(returnOldest(requestor), requestor) + (front_has_rd - 2) * tCCD + max(tRTW, 2 * tCCD) + tWtoR - 1 + tRL + tBUS;
								}
								tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor] : tempLatency;
								temp_timer[requestor] = 0;
							}
							if(cmdExistsHPA(RD, requestor, true)) 
							{
								temp_timer[requestor] = tRL + tBUS;
								tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor] : tempLatency;
								temp_timer[requestor] = 0;
							}
						}
						else if (returnOldest(requestor)->busPacketType == WR)
						{
							req_open[requestor] = true;

							if (isReadyTimer(returnOldest(requestor), requestor) <= 1)
							{
								if (CInterW == 0)
									temp_timer[requestor] = 1 + (REQ_count - 1) * tCCD + tRTW + tWL + tBUS;
								else
									temp_timer[requestor] = CInterW + (REQ_count - 1) * tCCD + tRTW + tWL + tBUS;
							}
							else
							{
								temp_timer[requestor] = isReadyTimer(returnOldest(requestor), requestor) + (REQ_count - 1) * tCCD + tRTW + 1 + tWL + tBUS;
							}
							tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor] : tempLatency;
							temp_timer[requestor] = 0;

							if(cmdExistsHPA(PRE, requestor, false))
							{
								if (isReadyTimer(returnOldest(requestor), requestor) <= 1)
								{
									if (CInterW == 0)
										temp_timer[requestor] = 1 + (REQ_count - 1) * tCCD + tRTW + tWL + tBUS;
									else
										temp_timer[requestor] = CInterW + (REQ_count - 1) * tCCD + tRTW + tWL + tBUS;
								}
								else
								{
									temp_timer[requestor] = isReadyTimer(returnOldest(requestor), requestor) + (REQ_count - 1) * tCCD + tRTW + 1 + tWL + tBUS;
								}
								tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor] : tempLatency;
								temp_timer[requestor] = 0;
							}
							if(cmdExistsHPA(ACT, requestor, false))
							{
								if (isReadyTimer(returnOldest(requestor), requestor) <= 1)
								{
									if (CInterW == 0)
										temp_timer[requestor] = 1 + (REQ_count - 1) * tCCD + tRTW + tWL + tBUS;
									else
										temp_timer[requestor] = CInterW + (REQ_count - 1) * tCCD + tRTW + tWL + tBUS;
								}
								else
								{
									temp_timer[requestor] = isReadyTimer(returnOldest(requestor), requestor) + (REQ_count - 1) * tCCD + tRTW + 1 + tWL + tBUS;
								}
								tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor] : tempLatency;
								temp_timer[requestor] = 0;
							}
							if(cmdExistsHPA(RD, requestor, false))
							{
								if (isReadyTimer(returnOldest(requestor), requestor) <= 1)
								{
									temp_timer[requestor] = tRTW + (REQ_count - 1) * tCCD + tRTW + tWL + tBUS;
								}
								else
								{
									temp_timer[requestor] = isReadyTimer(returnOldest(requestor), requestor) + (REQ_count - 1) * tCCD + tRTW + 1 + tWL + tBUS;
								}
								tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor] : tempLatency;
								temp_timer[requestor] = 0;
							}
							if(cmdExistsHPA(WR, requestor, false))
							{
								if (isReadyTimer(returnOldest(requestor), requestor) <= 1)
								{
									temp_timer[requestor] = tCCD + (REQ_count - 1) * tCCD + tRTW + tWL + tBUS;
								}
								else
								{
									temp_timer[requestor] = isReadyTimer(returnOldest(requestor), requestor) + (REQ_count - 1) * tCCD + tRTW + 1 + tWL + tBUS;
								}
								tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor] : tempLatency;
								temp_timer[requestor] = 0;
							}
							if(cmdExistsHPA(WR, requestor, true))
							{
								temp_timer[requestor] = tWL + tBUS;
								tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor] : tempLatency;
								temp_timer[requestor] = 0;
							}
						}
						else if (returnOldest(requestor)->busPacketType == PRE) // done modifications
						{
							// HPA can always send a NULL request
							req_open[requestor] = false;

							if (isReadyTimer(returnOldest(requestor), requestor) > 0)
								temp_timer[requestor] = isReadyTimer(returnOldest(requestor), requestor) + LPRE[front_has_pre] + tRP + LACT[front_has_act] + tRCD + (front_has_rd - 2) * tCCD + max(tRTW, 2 * tCCD) + tWtoR - 1 + tRL + tBUS;
							else if (isReadyTimer(returnOldest(requestor), requestor) == 0)
								temp_timer[requestor] = isReadyTimer(returnOldest(requestor), requestor) + LPRE[front_has_pre] + tRP + LACT[front_has_act] + tRCD + (front_has_rd - 2) * tCCD + max(tRTW, 2 * tCCD) + tWtoR - 1 + 1 + tRL + tBUS;

							tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor] : tempLatency;
							temp_timer[requestor] = 0;

							if(cmdExistsHPA(RD, requestor, true)) 
							{
								temp_timer[requestor] = max(isReadyTimer(returnOldest(requestor), requestor), tRTP) +
									LPRE[front_has_pre] + tRP + LACT[front_has_act] + tRCD + (front_has_rd - 2) * tCCD + max(tRTW, 2 * tCCD) + tWtoR - 1 + tRL + tBUS;
								tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor] : tempLatency;
								temp_timer[requestor] = 0;			
							}
							if(cmdExistsHPA(WR, requestor, true)) 
							{
								temp_timer[requestor] = max(isReadyTimer(returnOldest(requestor), requestor), tWR) + LPRE[front_has_pre] + tRP + LACT[front_has_act] + tRCD + (front_has_rd - 2) * tCCD + max(tRTW, 2 * tCCD) + tWtoR - 1 + tRL + tBUS;
								tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor] : tempLatency;
								temp_timer[requestor] = 0;		
							}
							if(cmdExistsHPA(ACT, requestor, true) || cmdExistsHPA(PRE, requestor, true)) 
							{
								temp_timer[requestor] = tRP + LACT[front_has_act] + tRCD + (front_has_rd - 2) * tCCD + max(tRTW, 2 * tCCD) + tWtoR - 1 + tRL + tBUS;
								tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor] : tempLatency;
								temp_timer[requestor] = 0;	

							}
							if(cmdExistsHPA(RD, requestor, false) || cmdExistsHPA(WR, requestor, false) || cmdExistsHPA(ACT, requestor, false) || cmdExistsHPA(PRE, requestor, false)) 
							{
								temp_timer[requestor] = isReadyTimer(returnOldest(requestor), requestor) + LPRE[front_has_pre] + tRP + LACT[front_has_act] + tRCD + (front_has_rd - 2) * tCCD + max(tRTW, 2 * tCCD) + tWtoR - 1 + tRL + tBUS;
								tempLatency = (tempLatency < temp_timer[requestor]) ? temp_timer[requestor] : tempLatency;
								temp_timer[requestor] = 0;	
							}
						}
					}
					else
					{
						temp_timer[requestor] = 0;
					}
				}
			}
			else if (impl == "C")
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
										if (CInterR == 0)
											temp_timer[requestor] = 1 + front_has_rd * tCCD + tRL + tBUS;
										else
											temp_timer[requestor] = CInterR + front_has_rd * tCCD + tRL + tBUS;
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
										if (CInterR == 0)
											temp_timer[requestor] = 1 + front_has_rd * tCCD + tRL + tBUS;
										else
											temp_timer[requestor] = CInterR + front_has_rd * tCCD + tRL + tBUS;
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
										if (CInterR == 0)
											temp_timer[requestor] = 1 + front_has_rd * tCCD + tRL + tBUS;
										else
											temp_timer[requestor] = CInterR + front_has_rd * tCCD + tRL + tBUS;
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
										if (CInterW == 0)
											temp_timer[requestor] = 1 + (REQ_count - 1) * tCCD + tRTW + tWL + tBUS;
										else
											temp_timer[requestor] = CInterW + (REQ_count - 1) * tCCD + tRTW + tWL + tBUS;
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
										if (CInterW == 0)
											temp_timer[requestor] = 1 + (REQ_count - 1) * tCCD + tRTW + tWL + tBUS;
										else
											temp_timer[requestor] = CInterW + (REQ_count - 1) * tCCD + tRTW + tWL + tBUS;
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
										if (CInterW == 0)
											temp_timer[requestor] = 1 + (REQ_count - 1) * tCCD + tRTW + tWL + tBUS;
										else
											temp_timer[requestor] = CInterW + (REQ_count - 1) * tCCD + tRTW + tWL + tBUS;
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
									if (CInterR == 0)
										temp_timer[requestor] = 1 + front_has_rd * tCCD + tRL + tBUS;
									else
										temp_timer[requestor] = CInterR + front_has_rd * tCCD + tRL + tBUS;
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
									if (CInterR == 0)
										temp_timer[requestor] = 1 + front_has_rd * tCCD + tRL + tBUS;
									else
										temp_timer[requestor] = CInterR + front_has_rd * tCCD + tRL + tBUS;
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
									if (CInterR == 0)
										temp_timer[requestor] = 1 + front_has_rd * tCCD + tRL + tBUS;
									else
										temp_timer[requestor] = CInterR + front_has_rd * tCCD + tRL + tBUS;
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
									if (CInterW == 0)
										temp_timer[requestor] = 1 + (REQ_count - 1) * tCCD + tRTW + tWL + tBUS;
									else
										temp_timer[requestor] = CInterW + (REQ_count - 1) * tCCD + tRTW + tWL + tBUS;
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
									if (CInterW == 0)
										temp_timer[requestor] = 1 + (REQ_count - 1) * tCCD + tRTW + tWL + tBUS;
									else
										temp_timer[requestor] = CInterW + (REQ_count - 1) * tCCD + tRTW + tWL + tBUS;
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
									if (CInterW == 0)
										temp_timer[requestor] = 1 + (REQ_count - 1) * tCCD + tRTW + tWL + tBUS;
									else
										temp_timer[requestor] = CInterW + (REQ_count - 1) * tCCD + tRTW + tWL + tBUS;
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
									if (CInterR == 0)
										temp_timer[requestor] = 1 + front_has_rd * tCCD + tRL + tBUS;
									else
										temp_timer[requestor] = CInterR + front_has_rd * tCCD + tRL + tBUS;
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
									if (CInterR == 0)
										temp_timer[requestor] = 1 + front_has_rd * tCCD + tRL + tBUS;
									else
										temp_timer[requestor] = CInterR + front_has_rd * tCCD + tRL + tBUS;
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
									if (CInterR == 0)
										temp_timer[requestor] = 1 + front_has_rd * tCCD + tRL + tBUS;
									else
										temp_timer[requestor] = CInterR + front_has_rd * tCCD + tRL + tBUS;
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
									if (CInterW == 0)
										temp_timer[requestor] = 1 + (REQ_count - 1) * tCCD + tRTW + tWL + tBUS;
									else
										temp_timer[requestor] = CInterW + (REQ_count - 1) * tCCD + tRTW + tWL + tBUS;
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
									if (CInterW == 0)
										temp_timer[requestor] = 1 + (REQ_count - 1) * tCCD + tRTW + tWL + tBUS;
									else
										temp_timer[requestor] = CInterW + (REQ_count - 1) * tCCD + tRTW + tWL + tBUS;
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
									if (CInterW == 0)
										temp_timer[requestor] = 1 + (REQ_count - 1) * tCCD + tRTW + tWL + tBUS;
									else
										temp_timer[requestor] = CInterW + (REQ_count - 1) * tCCD + tRTW + tWL + tBUS;
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

			/**
				Fetch the timing constraints.
			*/
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

			/**
				Write CAS to Read CAS timing.
			*/
			tWtoR = tBUS + tWTR + tWL;

			/**
				Manually calculate the RD/WR CAS inter-ready timers.
			*/
			CInterR = (CInterR > 0) ? (CInterR - 1) : 0;
			CInterW = (CInterW > 0) ? (CInterW - 1) : 0;			

			/**
				Send the command decided in the previous cycle from the register and remove from both queues.
			*/			
			if (mode == "RTA" && clock != 1)
			{
				if (scheduledCommand_RTA != NULL)
				{
					rta_count++;
					scheduledCommand = scheduledCommand_RTA;
					if (scheduledCommand->busPacketType == RD)
					{
						CInterR = tCCD;
						CInterW = tRTW;
						/*
						cout << "%%%%%%%%%%%%%%%%%%%%%%%%% RTA RD"
							 << "\t" << clock << ":"
							 << "\t\tAddress: " << scheduledCommand->address << "\tBank: " << scheduledCommand->bank << "\t\tColumn: " << scheduledCommand->column << "\tRow: " << scheduledCommand->row << endl;
							 */
					}
					else if (scheduledCommand->busPacketType == WR)
					{
						CInterR = tWtoR;
						CInterW = tCCD;
						/*
						cout << "%%%%%%%%%%%%%%%%%%%%%%%%% RTA WR"
							 << "\t" << clock << ":"
							 << "\t\tAddress: " << scheduledCommand->address << "\tBank: " << scheduledCommand->bank << "\t\tColumn: " << scheduledCommand->column << "\tRow: " << scheduledCommand->row << endl;
							 */
					}
					/*
					else if (scheduledCommand->busPacketType == ACT)
						cout << "%%%%%%%%%%%%%%%%%%%%%%%%% RTA ACT"
							 << "\t" << clock << ":"
							 << "\t\tAddress: " << scheduledCommand->address << "\tBank: " << scheduledCommand->bank << "\t\tColumn: " << scheduledCommand->column << "\tRow: " << scheduledCommand->row << endl;
					else if (scheduledCommand->busPacketType == PRE)
						cout << "%%%%%%%%%%%%%%%%%%%%%%%%% RTA PRE"
							 << "\t" << clock << ":"
							 << "\t\tAddress: " << scheduledCommand->address << "\tBank: " << scheduledCommand->bank << "\t\tColumn: " << scheduledCommand->column << "\tRow: " << scheduledCommand->row << endl;
					*/
					if (scheduledCommand->busPacketType < ACT)
					{
						servedFlags[scheduledCommand->bank] = true;
						req_statues_flag[scheduledCommand->requestorID] = false;
					}

					/**
						Sending command from RTA requires modification in the queues. 
					*/
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
					if (scheduledCommand->busPacketType == RD)
					{
						CInterR = tCCD;
						CInterW = tRTW;
						/*
						cout << "%%%%%%%%%%%%%%%%%%%%%%%%% HPA RD"
							 << "\t" << clock << ":"
							 << "\t\tAddress: " << scheduledCommand->address << "\tBank: " << scheduledCommand->bank << "\t\tColumn: " << scheduledCommand->column << "\tRow: " << scheduledCommand->row << endl;
							 */
					}
					else if (scheduledCommand->busPacketType == WR)
					{
						CInterR = tWtoR;
						CInterW = tCCD;
						/*
						cout << "%%%%%%%%%%%%%%%%%%%%%%%%% HPA WR"
							 << "\t" << clock << ":"
							 << "\t\tAddress: " << scheduledCommand->address << "\tBank: " << scheduledCommand->bank << "\t\tColumn: " << scheduledCommand->column << "\tRow: " << scheduledCommand->row << endl;
							 */
					}
					/*
					else if (scheduledCommand->busPacketType == ACT)
						cout << "%%%%%%%%%%%%%%%%%%%%%%%%% HPA ACT"
							 << "\t" << clock << ":"
							 << "\t\tAddress: " << scheduledCommand->address << "\tBank: " << scheduledCommand->bank << "\t\tColumn: " << scheduledCommand->column << "\tRow: " << scheduledCommand->row << endl;
					else if (scheduledCommand->busPacketType == PRE)
						cout << "%%%%%%%%%%%%%%%%%%%%%%%%% HPA PRE"
							 << "\t" << clock << ":"
							 << "\t\tAddress: " << scheduledCommand->address << "\tBank: " << scheduledCommand->bank << "\t\tColumn: " << scheduledCommand->column << "\tRow: " << scheduledCommand->row << endl;
					*/
					if (scheduledCommand->busPacketType < ACT)
					{
						req_statues_flag[scheduledCommand->requestorID] = false;
					}

					/**
						Sending command from HPA does not require modification in the queues. 
					*/
					sendCommand(scheduledCommand, scheduledCommand->bank, false);
				}
			}

			/**
				Set the deadline for the oldest request of each requestor if required. 
			*/
			setDeadline();

			/*
			cout << "||||||||||||||||||||||||||||| Order Watches ||||||||||||||||||||||||" << endl;
			if (Order.size() != 0)
			{
				for (unsigned int h = 0; h < Order.size(); h++)
				{
					cout << " Order watch:  " << Order.at(h) << endl;
				}
			}
			
			cout << "||||||||||||||||||||||||||||| RT Queue Watches ||||||||||||||||||||||||" << endl;
			for (unsigned int i = 0; i < commandQueue_RT.size(); i++)
			{
				if (commandQueue_RT[i]->getRequestorIndex() > 0) // There is more than 0 requestor in the design
				{
					for (unsigned int k = 0; k < 8; k++)
					{
						if (commandQueue_RT[i]->getRequestorSize(k, true) > 0) // Return the buffer size of the requestor
						{
							cout << " RT Queue of  " << k << "  contains   " << commandQueue_RT[i]->getRequestorCommand(k, true)->busPacketType << "  address   " << commandQueue_RT[i]->getRequestorCommand(k, true)->address << endl;
						}
					}
				}
			}

			cout << "||||||||||||||||||||||||||||| HP Queue Watches ||||||||||||||||||||||||" << endl;
			for (unsigned int i = 0; i < commandQueue.size(); i++)
			{
				if (commandQueue[i]->getRequestorIndex() > 0) // There is more than 0 requestor in the design
				{
					for (unsigned int k = 0; k < 8; k++)
					{
						if (commandQueue[i]->getRequestorSize(k, false) > 0) // Return the buffer size of the requestor
						{
							cout << " HP Queue of  " << k << "  contains   " << commandQueue[i]->getRequestorCommand(k, false)->busPacketType << "  address   " << commandQueue[i]->getRequestorCommand(k, false)->address << endl;
						}
					}
				}
			}
			cout << "|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||" << endl;

			*/

			for (unsigned int i = 0; i < REQ_count; i++)
			{
				if (service_deadline[i] > 0)
					service_deadline[i]--;
				else
					service_deadline[i] = 0;
			}

			/*
			cout << "||||||||||||||||||||||||||||| Deadline Watches ||||||||||||||||||||||" << endl;
			for (unsigned int i = 0; i < bank_count; i++)
			{
				cout << "the deadline for   " << i << "   is   " << service_deadline[i] << endl;
			}
			*/

			checkCommand = NULL;
			checkCommand_temp_1 = NULL;
			checkCommand_temp_2 = NULL;
			scheduledCommand_HPA = NULL;
			scheduledCommand_RTA = NULL;

			/**
				HPA tries to schedule a command to issue. 
				First, it looks for oldest CAS command.		 
			*/
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
			
			/**
				HPA tries to schedule a command to issue. 
				If no CAS exists, try to schedule a PRE/ACT command.
			*/
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
						if (commandQueue[index]->getRequestorIndex() > 0) // There is more than 0 requestor in the designisReady
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

			/**
				Decision needs to be made now (it is simulation decision to accomodate different implementions).
				Keep the prevous mode to further use of fixing the queues.
			*/		
			mode_prev = mode;  
			
			/**
				If in HPA, and at least one requestor is at risk of missing its deadline; switch to RTA.
				If in RTA, and at least one requestor is at risk of missing its deadline; remains in RTA.
				If in RTA, and there is not risk for all requestor of missing trheir deadline; switch to HPA.				
			*/
			if (mode == "HPA")
			{
				for (unsigned int index = 0; index < REQ_count; index++)
				{
					if (WCLator(index, scheduledCommand_HPA) >= service_deadline[index])
					{
						cout << "-----------------------------------------SWITCH TO RTA-------------------------------------------- Because of " << index << " latency " << WCLator(index, scheduledCommand_HPA) << endl;
						suspect_requestor = index;
						suspect_flag = true;
						mode = "RTA";
						break;
					}
				}
			}
			else
			{
				for (unsigned int index = 0; index < REQ_count; index++)
				{
					if (WCLator(index, scheduledCommand_HPA) < service_deadline[index])
					{
						cout << "-----------------------------------------SWITCH TO HPA-------------------------------------------- Because of " << index << " latency " << WCLator(index, scheduledCommand_HPA) << endl;
						mode = "HPA";
					}
					else
					{
						cout << "-----------------------------------------REMAIN IN RTA-------------------------------------------- Becaus of " << index << " latency " << WCLator(index, scheduledCommand_HPA) << endl;
						mode = "RTA";
						break;
					}
				}
			}

			scheduledCommand_RTA = NULL;

			/**
				RTA tries to schedule a command to issue. 
			*/
			if (mode == "RTA")
			{
				/**
					Scan the command queue for ready CAS. 
				*/
				for (unsigned int index = 0; index < commandQueue_RT.size(); index++) 
				{
					/**
						For all requestors from "num". getRequestorIndex() gives the number of requestors. 
					*/
					for (unsigned int num = 0; num < commandQueue_RT[index]->getRequestorIndex(); num++) 
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
					}
					checkCommand = NULL;
				}

				registerIndex = 0;

				/**
					First Level of Arbitration among oldest of each requestor.
				*/				
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

				/**
					Second Level of Arbitration among non oldest of each requestor.
				*/								
				if (scheduledCommand_RTA == NULL)
				{
					/**
						Essentially if happens here anything, it means that a new round has been started.
					*/						
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

				/**
					Round-Robin ACT arbitration.
				*/					
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

				/**
					Round-Robin PRE arbitration.
				*/					
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

				/**
					Shared bank control.
				*/					
				if (scheduledCommand != NULL)
				{
					if (scheduledCommand->busPacketType == RD || scheduledCommand->busPacketType == WR)
					{						
						if(isShared(scheduledCommand->bank))
						{													
							active_shared_exist = false;
							active_shared_bank = 1000;
							active_shared_req  = 1000;
						}
					}
					else if (scheduledCommand->busPacketType == PRE && isShared(scheduledCommand->bank))
					{						
						active_shared_exist = true;
						active_shared_bank = scheduledCommand->bank;
						active_shared_req  = scheduledCommand->requestorID;
					}					
				}
			}

			/**
				If RTA/HPA was previous and now RTA, and it tries to close a bank, the HPA queue should be updated. 
			*/
			if (mode == "RTA") 
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

			/**
				Remove anything remained in the RT command registers if going to HPA. These registers are specificly for the RTA controller. 
			*/
			if (mode_prev == "RTA" && mode == "HPA") 
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
			
			/**
				Maintaining the RTA queues if HPA is chosen.
				Maintaining the HPA queues if RTA is chosen.
			*/
			if (mode == "HPA")
			{
				if (scheduledCommand_HPA != NULL)
				{
					if (scheduledCommand_HPA->busPacketType == RD || scheduledCommand_HPA->busPacketType == WR)
					{
						requestQueues_local[0]->updateRowTable(scheduledCommand_HPA->rank, scheduledCommand_HPA->bank, scheduledCommand_HPA->row);
						if (returnOldest(scheduledCommand_HPA->requestorID)->busPacketType == scheduledCommand_HPA->busPacketType && returnOldest(scheduledCommand_HPA->requestorID)->address == scheduledCommand_HPA->address)
						{																											
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
			
			return scheduledCommand;
		}
	};
}
#endif /*  */
