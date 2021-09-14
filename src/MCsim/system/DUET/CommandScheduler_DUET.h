
#ifndef COMMANDSCHEDULER_DUET_H
#define COMMANDSCHEDULER_DUET_H

#include "../../src/CommandScheduler.h"


namespace MCsim
{
	class CommandScheduler_DUET: public CommandScheduler
	{
	private:
			vector<RequestQueue*>& requestQueues_local;
			
			unsigned int registerIndex;
			unsigned int temp;
			string mode; //RTMode, HPMode
			string mode_prev; //RTMode, HPMode
			bool start;
			vector<std::pair<BusPacket*, unsigned int>> commandRegisters;
			vector<bool> servedFlags;
			vector<bool> queuePending;
			vector<bool> virChannelRegister; 
			vector<unsigned int> Order;
			vector<unsigned int> Order_temp;
			unsigned int bank_count;
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
			int suspect_requestor;
			bool suspect_flag;
			 int front_has_sth;
			 int front_has_pre;
			 int front_has_act;
			 int front_has_rd;
			 int front_has_wr;
			bool remove;
	public:
		CommandScheduler_DUET(vector<RequestQueue*>& requestQueues, vector<CommandQueue*>& commandQueues, 
				const map<unsigned int, bool>& requestorTable,  vector<CommandQueue*>& commandQueue_RT):
			CommandScheduler(commandQueues, requestorTable, commandQueue_RT),
			requestQueues_local(requestQueues)
			{		
				roundType = BusPacketType::RD;	// Read Type
				indexCAS = 0;
				indexACT = 0;
				indexPRE = 0;	
				front_has_sth = 0;
				front_has_pre =0;
				front_has_act =0;
				front_has_rd =0;
				front_has_wr =0;
				tCCD = 4;
				total_read = 0;
				sum_wcl = 0;
			count = 0;
				total_write = 0;
				suspect_flag = false;
				//tWL  = getTiming("tWL");
			
				bank_count = 8;
				init_deadline = 150;//90;//45;
				mode = "HPMode";
				start = true;
				flag_deadline = false;
				for (unsigned int i = 0; i< bank_count ; i++)
					deadline_set_flag[i] = false;	
				for (unsigned int i = 0; i< bank_count ; i++)
					req_open[i] = false;
				for (unsigned int i = 0; i< bank_count ; i++)
					req_statues_flag[i] = false;
				//for (unsigned int i = 0; i < bank_count; i++)
				//	Order.push_back(i);		
				for(unsigned index=0; index < bank_count; index++)
					service_deadline[index] = init_deadline;
				for(unsigned index=0; index < bank_count; index++) 
					queuePending.push_back(false);			
				for(unsigned index=0; index < bank_count; index++) 
					servedFlags.push_back(false);	
				
			}
			unsigned int actual_wcl(unsigned int requestor, BusPacket* cmd)
			{
				unsigned int LACT[8] = {0,6,12,18,24,30,36,42};
				unsigned int LPRE[8] = {0,2,4,6,8,10,12,14};
				
				front_has_sth = 0;
				front_has_pre =0;
				front_has_act =0;
				front_has_rd =0;
				front_has_wr =0;
				int N = 0;
				for(unsigned int i = 0; i < Order.size(); i++)
				{
					if(Order.at(i) == requestor)					
						N = i;	// number of requestors that are ahead of this specific requestor
				}
				Order_temp = Order;
				if(cmd != NULL)
				{
					if (cmd->busPacketType == RD || cmd->busPacketType == WR)
					{
						for(unsigned int h=0; h<Order_temp.size() ; h++){
							if(Order_temp.at(h) == cmd->requestorID)
							{
								Order_temp.erase(Order_temp.begin() + h);
								Order_temp.push_back(cmd->requestorID);
								break;
							}
						}	
						for(unsigned int i = 0; i < Order_temp.size(); i++)
						{
							if(Order_temp.at(i) == requestor)					
								N = i;	// number of requestors that are ahead of this specific requestor
						}
					}
				}
				// number of in front and has sth
				for (int traverse = 0; traverse < N ;traverse++)
				{
					if(commandQueue_RT[traverse]->getSize(true) > 0)
					{
						front_has_sth++;
					}
				}
				// number of in front and has PRE
				for ( int traverse = 0; traverse < N ;traverse++)
				{
					if(commandQueue_RT[traverse]->getSize(true) > 0)
					{
						if(commandQueue_RT[traverse]->getCommand(true)->busPacketType == PRE)
						{
							front_has_pre++;
						}
					}
				}
				// number of in front and has ACT
				for ( int traverse = 0; traverse < N ;traverse++)
				{
					if(commandQueue_RT[traverse]->getSize(true) > 0)
					{
						if(commandQueue_RT[traverse]->getCommand(true)->busPacketType == PRE)
						{
							if(commandQueue_RT[traverse]->checkCommand(true,1)->busPacketType == ACT){
								front_has_act++;
							}
						}
						else if(commandQueue_RT[traverse]->getCommand(true)->busPacketType == ACT)
						{
							front_has_act++;
						}
					}
				}
				// number of in front and is read
				for ( int traverse = 0; traverse < N ;traverse++)
				{
					if(commandQueue_RT[traverse]->getSize(true) > 0)
					{
						if(commandQueue_RT[traverse]->getCommand(true)->busPacketType == PRE)
						{
							if(commandQueue_RT[traverse]->checkCommand(true,2)->busPacketType == RD)
							{
								front_has_rd++;
							}							
						}
						else if(commandQueue_RT[traverse]->getCommand(true)->busPacketType == ACT)
						{
							if(commandQueue_RT[traverse]->checkCommand(true,1)->busPacketType == RD)
							{
								front_has_rd++;
							}	
						}
						else if(commandQueue_RT[traverse]->getCommand(true)->busPacketType == RD)
						{
							front_has_rd++;
						}
					}
				}
				// number of in front and has write  
				for ( int traverse = 0; traverse < N ;traverse++)
				{
					if(commandQueue_RT[traverse]->getSize(true) > 0)
					{
						if(commandQueue_RT[traverse]->getCommand(true)->busPacketType == PRE)
						{
							if(commandQueue_RT[traverse]->checkCommand(true,2)->busPacketType == WR)
							{
								front_has_wr++;
							}							
						}
						else if(commandQueue_RT[traverse]->getCommand(true)->busPacketType == ACT)
						{
							if(commandQueue_RT[traverse]->checkCommand(true,1)->busPacketType == WR)
							{
								front_has_wr++;
							}	
						}
						else if(commandQueue_RT[traverse]->getCommand(true)->busPacketType == WR)
						{
							front_has_wr++;
						}
					}
				}
				if(N-1 < 0){
					LPRE[N-1] = 0;
					LACT[N-1] = 0;}
				
				if (req_statues_flag[requestor] == true)
				{	
					if(req_open[requestor] == true)
					{
						if(commandQueue_RT[requestor]->getSize(true) > 0)
						{
							if(commandQueue_RT[requestor]->getCommand(true)->busPacketType == RD)
							{
								if(cmd == NULL)
								{
									temp_timer[requestor] = isReadyTimer(commandQueue_RT[requestor]->getCommand(true),requestor) + max(tWtoR - tCCD + N*tCCD + 1 + tRL + tBUS, tRTW - tCCD  + (bank_count-2)*tCCD + tWtoR + 1 + tRL + tBUS);
								// return open read wcl
								}
								else
								{
									if(cmd->requestorID == commandQueue_RT[requestor]->getCommand(true)->requestorID)
										temp_timer[requestor] = 1;
									else																	
									{
										if(cmd->busPacketType == RD)
										{
											if(isReadyTimer(commandQueue_RT[requestor]->getCommand(true),requestor) > tCCD){
											
												isReadyTimer(commandQueue_RT[requestor]->getCommand(true),requestor) + max(tWtoR - tCCD + N*tCCD + 1 + tRL + tBUS, tRTW - tCCD  + (bank_count-2)*tCCD + tWtoR + 1 + tRL + tBUS);
											}
											else{
												tCCD + max(tWtoR - tCCD + N*tCCD + 1 + tRL + tBUS, tRTW - tCCD  + (bank_count-2)*tCCD + tWtoR + 1 + tRL + tBUS);								
											}
										}
										else if (cmd->busPacketType == WR)
										{
											if(isReadyTimer(commandQueue_RT[requestor]->getCommand(true),requestor) > tWtoR){
											
												isReadyTimer(commandQueue_RT[requestor]->getCommand(true),requestor) + max(tWtoR - tCCD + N*tCCD + 1 + tRL + tBUS, tRTW - tCCD  + (bank_count-2)*tCCD + tWtoR + 1 + tRL + tBUS);
											}
											else {
												tWtoR + max(tWtoR - tCCD + N*tCCD + 1 + tRL + tBUS, tRTW - tCCD  + (bank_count-2)*tCCD + tWtoR + 1 + tRL + tBUS);
											}
										}
									}																	
								}
							}
							else if (commandQueue_RT[requestor]->getCommand(true)->busPacketType == WR)
							{
								if(cmd == NULL)
								{
									temp_timer[requestor] = isReadyTimer(commandQueue_RT[requestor]->getCommand(true),requestor) + max(tRTW - tCCD + N*tCCD + 1 + tWL + tBUS, tWtoR - tCCD  + (bank_count-2)*tCCD + tRTW + 1 + tWL + tBUS);
									// return open read wcl
								}
								else
								{
									if(cmd->requestorID == commandQueue_RT[requestor]->getCommand(true)->requestorID){
								
										temp_timer[requestor] = 1;
									}
									else																	
									{
										if(cmd->busPacketType == WR)
										{
											if(isReadyTimer(commandQueue_RT[requestor]->getCommand(true),requestor) > tCCD){
								
												isReadyTimer(commandQueue_RT[requestor]->getCommand(true),requestor) + max(tRTW - tCCD + N*tCCD + 1 + tWL + tBUS, tWtoR - tCCD  + (bank_count-2)*tCCD + tRTW + 1 + tWL + tBUS);
											}
											else
											{
												tCCD + max(tRTW - tCCD + N*tCCD + 1 + tWL + tBUS, tWtoR - tCCD  + (bank_count-2)*tCCD + tRTW + 1 + tWL + tBUS);								
											}
										}
										else if (cmd->busPacketType == RD)
										{
											if(isReadyTimer(commandQueue_RT[requestor]->getCommand(true),requestor) > tRTW){
									
												isReadyTimer(commandQueue_RT[requestor]->getCommand(true),requestor) + max(tRTW - tCCD + N*tCCD + 1 + tWL + tBUS, tWtoR - tCCD  + (bank_count-2)*tCCD + tRTW + 1 + tWL + tBUS);
											}
											else{ 
												tRTW + max(tRTW - tCCD + N*tCCD + 1 + tWL + tBUS, tWtoR - tCCD  + (bank_count-2)*tCCD + tRTW + 1 + tWL + tBUS);
												
											}
										}
									}																	
								}
							}							
						}
					}
					else if (req_open[requestor] == false)
					{
						if(commandQueue_RT[requestor]->getCommand(true)->busPacketType == PRE)
						{
							
							// no change return just the wcl for close read or close wr 
							if(cmd != NULL)
							{
								if(cmd->requestorID == requestor)
								{
									if(cmd->busPacketType == RD)
									{
										if(commandQueue_RT[requestor]->checkCommand(true,2)->busPacketType == RD)
										{
											
											if(isReadyTimer(commandQueue_RT[requestor]->getCommand(true),requestor) > tRTP)
											{
												temp_timer[requestor] = isReadyTimer(commandQueue_RT[requestor]->getCommand(true),requestor) + max(LPRE[front_has_pre] + tRP + LACT[front_has_act] + tRCD + (tWtoR - tCCD) + N*tCCD + 1 + tRL + tBUS, LPRE[front_has_pre] 
													+ tRP + LACT[front_has_act] + tRCD + (tRTW-tCCD) + (bank_count-2)*tCCD + tWtoR +1 + tRL + tBUS);
											}
											else
											{
												temp_timer[requestor] = tRTP + max(LPRE[front_has_pre] + tRP + LACT[front_has_act] + tRCD + (tWtoR - tCCD) + N*tCCD + 1 + tRL + tBUS, LPRE[front_has_pre] 
													+ tRP + LACT[front_has_act] + tRCD + (tRTW-tCCD) + (bank_count-2)*tCCD + tWtoR +1 + tRL + tBUS);
											}
										}
										else if(commandQueue_RT[requestor]->checkCommand(true,2)->busPacketType == WR)
										{
											if(isReadyTimer(commandQueue_RT[requestor]->getCommand(true),requestor) > tRTP)
											{
												temp_timer[requestor] =  isReadyTimer(commandQueue_RT[requestor]->getCommand(true),requestor) + max(LPRE[front_has_pre] + tRP + LACT[front_has_act] + tRCD + (tRTW - tCCD) + N*tCCD + 1 + tWL + tBUS, LPRE[front_has_pre] + 
													tRP + LACT[front_has_act] + tRCD + (tWtoR-tCCD) + (bank_count-2)*tCCD + tRTW + 1 + tWL + tBUS);
											}
											else
											{
												temp_timer[requestor] =  tRTP + max(LPRE[front_has_pre] + tRP + LACT[front_has_act] + tRCD + (tRTW - tCCD) + N*tCCD + 1 + tWL + tBUS, LPRE[front_has_pre] + 
													tRP + LACT[front_has_act] + tRCD + (tWtoR-tCCD) + (bank_count-2)*tCCD + tRTW + 1 + tWL + tBUS);
											}
											
										}

									}
									else if(cmd->busPacketType == WR)
									{
										if(commandQueue_RT[requestor]->checkCommand(true,2)->busPacketType == RD)
										{
											if(isReadyTimer(commandQueue_RT[requestor]->getCommand(true),requestor) > tWR)
											{
												temp_timer[requestor] = isReadyTimer(commandQueue_RT[requestor]->getCommand(true),requestor) + max(LPRE[front_has_pre] + tRP + LACT[front_has_act] + tRCD + (tWtoR - tCCD) + N*tCCD + 1 + tRL + tBUS, LPRE[front_has_pre] + 
													tRP + LACT[front_has_act] + tRCD + (tRTW-tCCD) + (bank_count-2)*tCCD + tWtoR +1 + tRL + tBUS);
											}
											else
											{
												temp_timer[requestor] = tWR + tWL + tBUS + max(LPRE[front_has_pre] + tRP + LACT[front_has_act] + tRCD + (tWtoR - tCCD) + N*tCCD + 1 + tRL + tBUS, LPRE[front_has_pre] + 
													tRP + LACT[front_has_act] + tRCD + (tRTW-tCCD) + (bank_count-2)*tCCD + tWtoR +1 + tRL + tBUS);
											}										
										}
										if(commandQueue_RT[requestor]->checkCommand(true,2)->busPacketType == WR)
										{
										
											if(isReadyTimer(commandQueue_RT[requestor]->getCommand(true),requestor) > tWR)
											{
												temp_timer[requestor] =  isReadyTimer(commandQueue_RT[requestor]->getCommand(true),requestor) + max(LPRE[front_has_pre] + tRP + LACT[front_has_act] + tRCD + (tRTW - tCCD) + N*tCCD + 1 + tWL + tBUS, LPRE[front_has_pre] + 
													tRP + LACT[front_has_act] + tRCD + (tWtoR-tCCD) + (bank_count-2)*tCCD + tRTW + 1 + tWL + tBUS);
											}
											else
											{
												temp_timer[requestor] =   tWR + tWL + tBUS + max(LPRE[front_has_pre] + tRP + LACT[front_has_act] + tRCD + (tRTW - tCCD) + N*tCCD + 1 + tWL + tBUS, LPRE[front_has_pre] + 
													tRP + LACT[front_has_act] + tRCD + (tWtoR-tCCD) + (bank_count-2)*tCCD + tRTW + 1 + tWL + tBUS);
											}							
										}
									}
									
								}
								else
								{
									if(commandQueue_RT[requestor]->checkCommand(true,2)->busPacketType == RD)
									{
										temp_timer[requestor] = isReadyTimer(commandQueue_RT[requestor]->getCommand(true),requestor) + max(LPRE[front_has_pre] + tRP + LACT[front_has_act] + tRCD + (tWtoR - tCCD) + N*tCCD + 1 + tRL + tBUS, LPRE[front_has_pre] 
											+ tRP + LACT[front_has_act] + tRCD + (tRTW-tCCD) + (bank_count-2)*tCCD + tWtoR +1 + tRL + tBUS);
									}
									else if(commandQueue_RT[requestor]->checkCommand(true,2)->busPacketType == WR)
									{
										temp_timer[requestor] = isReadyTimer(commandQueue_RT[requestor]->getCommand(true),requestor) + max(LPRE[front_has_pre] + tRP + LACT[front_has_act] + tRCD + (tRTW - tCCD) + N*tCCD + 1 + tWL + tBUS, LPRE[front_has_pre] 
											+ tRP + LACT[front_has_act] + tRCD + (tWtoR-tCCD) + (bank_count-2)*tCCD + tRTW + 1 + tWL + tBUS);
									}
								}
							}
							else
							{
								if(commandQueue_RT[requestor]->checkCommand(true,2)->busPacketType == RD)
								{
									temp_timer[requestor] = isReadyTimer(commandQueue_RT[requestor]->getCommand(true),requestor) + max(LPRE[front_has_pre] + tRP + LACT[front_has_act] + tRCD + (tWtoR - tCCD) + N*tCCD + 1 + tRL + tBUS, LPRE[front_has_pre] 
										+ tRP + LACT[front_has_act] + tRCD + (tRTW-tCCD) + (bank_count-2)*tCCD + tWtoR +1 + tRL + tBUS);
								}
								else if(commandQueue_RT[requestor]->checkCommand(true,2)->busPacketType == WR)
								{
									temp_timer[requestor] = isReadyTimer(commandQueue_RT[requestor]->getCommand(true),requestor) + max(LPRE[front_has_pre] + tRP + LACT[front_has_act] + tRCD + (tRTW - tCCD) + N*tCCD + 1 + tWL + tBUS, LPRE[front_has_pre] 
										+ tRP + LACT[front_has_act] + tRCD + (tWtoR-tCCD) + (bank_count-2)*tCCD + tRTW + 1 + tWL + tBUS);
								}
							}
								
						}
						else if(commandQueue_RT[requestor]->getCommand(true)->busPacketType == ACT)
						{
							if(commandQueue_RT[requestor]->checkCommand(true,1)->busPacketType == RD)
							{
								temp_timer[requestor] = max(LPRE[front_has_pre] + tRP + LACT[front_has_act] + (tWtoR - tCCD) + N*tCCD + 1 + tRL + tBUS, LPRE[front_has_pre] 
									+ tRP + LACT[front_has_act] + (tRTW-tCCD) + (bank_count-2)*tCCD + tWtoR +1 + tRL + tBUS) - LPRE[front_has_pre] + isReadyTimer(commandQueue_RT[requestor]->getCommand(true),requestor);
							}
							else if(commandQueue_RT[requestor]->checkCommand(true,1)->busPacketType == WR)
							{
								temp_timer[requestor] = max(LPRE[front_has_pre] + tRP + LACT[front_has_act] + (tRTW - tCCD) + N*tCCD + 1 + tWL + tBUS, LPRE[front_has_pre] 
									+ tRP + LACT[front_has_act] + (tWtoR-tCCD) + (bank_count-2)*tCCD + tRTW + 1 + tWL + tBUS) - LPRE[front_has_pre] +  isReadyTimer(commandQueue_RT[requestor]->getCommand(true),requestor);
							}							
						}
						else if (commandQueue_RT[requestor]->getCommand(true)->busPacketType == RD)
						{
							// WCL - LPRE - tRP - LACT - Lremaining(CAS)
							temp_timer[requestor] = max(LPRE[front_has_pre] + tRP + LACT[front_has_act]  + (tWtoR - tCCD) + N*tCCD + 1 + tRL + tBUS, LPRE[front_has_pre] + tRP + LACT[front_has_act] 
								+ (tRTW-tCCD) + (bank_count-2)*tCCD + tWtoR +1 + tRL + tBUS) - LPRE[front_has_pre] - tRP - LACT[front_has_act] +  isReadyTimer(commandQueue_RT[requestor]->getCommand(true),requestor);
						}
						else if (commandQueue_RT[requestor]->getCommand(true)->busPacketType == WR)
						{
							/// WCL - LPRE - tRP - LACT - Lremaining(CAS)
							temp_timer[requestor] = max(LPRE[front_has_pre] + tRP + LACT[front_has_act]  + (tRTW - tCCD) + N*tCCD + 1 + tWL + tBUS, LPRE[front_has_pre] + tRP + LACT[front_has_act]  
								+ (tWtoR-tCCD) + (bank_count-2)*tCCD + tRTW + 1 + tWL + tBUS) - LPRE[front_has_pre] - tRP - LACT[front_has_act] +  isReadyTimer(commandQueue_RT[requestor]->getCommand(true),requestor);
						}
					}				
				}
				else if (req_statues_flag[requestor] == false)
				{
					if(commandQueue_RT[requestor]->getSize(true) > 0)
					{
						req_statues_flag[requestor] = true;						
						if(commandQueue_RT[requestor]->getCommand(true)->busPacketType == RD)
						{
							
							req_open[requestor] = true;
							if(cmd == NULL)
							{
								temp_timer[requestor] = isReadyTimer(commandQueue_RT[requestor]->getCommand(true),requestor) + max(tWtoR - tCCD + N*tCCD + 1 + tRL + tBUS, tRTW - tCCD  + (bank_count-2)*tCCD + tWtoR + 1 + tRL + tBUS);
							// return open read wcl
							}
							else
							{
								if(cmd->requestorID == commandQueue_RT[requestor]->getCommand(true)->requestorID){
									temp_timer[requestor] = 1;
								
								}
								else																	
								{
									if(cmd->busPacketType == RD)
									{
										if(isReadyTimer(commandQueue_RT[requestor]->getCommand(true),requestor) > tCCD)
											isReadyTimer(commandQueue_RT[requestor]->getCommand(true),requestor) + max(tWtoR - tCCD + N*tCCD + 1 + tRL + tBUS, tRTW - tCCD  + (bank_count-2)*tCCD + tWtoR + 1 + tRL + tBUS);
										else
											tCCD + max(tWtoR - tCCD + N*tCCD + 1 + tRL + tBUS, tRTW - tCCD  + (bank_count-2)*tCCD + tWtoR + 1 + tRL + tBUS);								
									}
									else if (cmd->busPacketType == WR)
									{
										if(isReadyTimer(commandQueue_RT[requestor]->getCommand(true),requestor) > tWtoR)
											isReadyTimer(commandQueue_RT[requestor]->getCommand(true),requestor) + max(tWtoR - tCCD + N*tCCD + 1 + tRL + tBUS, tRTW - tCCD  + (bank_count-2)*tCCD + tWtoR + 1 + tRL + tBUS);
										else 
											tWtoR + max(tWtoR - tCCD + N*tCCD + 1 + tRL + tBUS, tRTW - tCCD  + (bank_count-2)*tCCD + tWtoR + 1 + tRL + tBUS);
									}
								}																	
							}							
						}
						else if (commandQueue_RT[requestor]->getCommand(true)->busPacketType == WR)
						{
							req_open[requestor] = true;
							if(cmd == NULL)
							{
								
								temp_timer[requestor] = isReadyTimer(commandQueue_RT[requestor]->getCommand(true),requestor) + max(tRTW - tCCD + N*tCCD + 1 + tWL + tBUS, tWtoR - tCCD  + (bank_count-2)*tCCD + tRTW + 1 + tWL + tBUS);
								// return open read wcl
							}
							else
							{
								if(cmd->requestorID == commandQueue_RT[requestor]->getCommand(true)->requestorID)
									temp_timer[requestor] = 1;
								else																	
								{
									if(cmd->busPacketType == WR)
									{
										if(isReadyTimer(commandQueue_RT[requestor]->getCommand(true),requestor) > tCCD)
											isReadyTimer(commandQueue_RT[requestor]->getCommand(true),requestor) + max(tRTW - tCCD + N*tCCD + 1 + tWL + tBUS, tWtoR - tCCD  + (bank_count-2)*tCCD + tRTW + 1 + tWL + tBUS);
										else
											tCCD + max(tRTW - tCCD + N*tCCD + 1 + tWL + tBUS, tWtoR - tCCD  + (bank_count-2)*tCCD + tRTW + 1 + tWL + tBUS);								
									}
									else if (cmd->busPacketType == RD)
									{
										if(isReadyTimer(commandQueue_RT[requestor]->getCommand(true),requestor) > tRTW)
											isReadyTimer(commandQueue_RT[requestor]->getCommand(true),requestor) + max(tRTW - tCCD + N*tCCD + 1 + tWL + tBUS, tWtoR - tCCD  + (bank_count-2)*tCCD + tRTW + 1 + tWL + tBUS);
										else 
											tRTW + max(tRTW - tCCD + N*tCCD + 1 + tWL + tBUS, tWtoR - tCCD  + (bank_count-2)*tCCD + tRTW + 1 + tWL + tBUS);
									}
								}																	
							}
						}
						else if (commandQueue_RT[requestor]->getCommand(true)->busPacketType == PRE)
						{
							if(cmd != NULL)
							{
								if(cmd->requestorID == requestor)
								{
									if(cmd->busPacketType == RD)
									{
										req_open[requestor] = false;
										if(commandQueue_RT[requestor]->checkCommand(true,2)->busPacketType == RD)
										{
											/// return close read wcl
											if(isReadyTimer(commandQueue_RT[requestor]->getCommand(true),requestor) > tRTP)
											{
												//***************
												temp_timer[requestor] = isReadyTimer(commandQueue_RT[requestor]->getCommand(true),requestor) + max(LPRE[front_has_pre] + tRP + LACT[front_has_act] + tRCD + (tWtoR - tCCD) + N*tCCD + 1 + tRL + tBUS, LPRE[front_has_pre] + 
													tRP + LACT[front_has_act] + tRCD + (tRTW-tCCD) + (bank_count-2)*tCCD + tWtoR +1 + tRL + tBUS);
											}
											else
											{
												temp_timer[requestor] = tRTP + max(LPRE[front_has_pre] + tRP + LACT[front_has_act] + tRCD + (tWtoR - tCCD) + N*tCCD + 1 + tRL + tBUS, LPRE[front_has_pre] + 
													tRP + LACT[front_has_act] + tRCD + (tRTW-tCCD) + (bank_count-2)*tCCD + tWtoR +1 + tRL + tBUS);
											}
										}
										else if(commandQueue_RT[requestor]->checkCommand(true,2)->busPacketType == WR)
										{
											if(isReadyTimer(commandQueue_RT[requestor]->getCommand(true),requestor) > tRTP)
											{
												temp_timer[requestor] =  isReadyTimer(commandQueue_RT[requestor]->getCommand(true),requestor) + max(LPRE[front_has_pre] + tRP + LACT[front_has_act] + tRCD + (tRTW - tCCD) + N*tCCD + 1 + tWL + tBUS, LPRE[front_has_pre] + 
													tRP + LACT[front_has_act] + tRCD + (tWtoR-tCCD) + (bank_count-2)*tCCD + tRTW + 1 + tWL + tBUS);
											}
											else
											{
												temp_timer[requestor] =  tRTP + max(LPRE[front_has_pre] + tRP + LACT[front_has_act] + tRCD + (tRTW - tCCD) + N*tCCD + 1 + tWL + tBUS, LPRE[front_has_pre] + 
													tRP + LACT[front_has_act] + tRCD + (tWtoR-tCCD) + (bank_count-2)*tCCD + tRTW + 1 + tWL + tBUS);
											}
										}
									}
									if(cmd->busPacketType == WR)
									{
										req_open[requestor] = false;
										if(commandQueue_RT[requestor]->checkCommand(true,2)->busPacketType == RD)
										{
											if(isReadyTimer(commandQueue_RT[requestor]->getCommand(true),requestor) > tWR)
											{
												temp_timer[requestor] = isReadyTimer(commandQueue_RT[requestor]->getCommand(true),requestor) + max(LPRE[front_has_pre] + tRP + LACT[front_has_act] + tRCD + (tWtoR - tCCD) + N*tCCD + 1 + tRL + tBUS, LPRE[front_has_pre] + 
													tRP + LACT[front_has_act] + tRCD + (tRTW-tCCD) + (bank_count-2)*tCCD + tWtoR +1 + tRL + tBUS);
											}
											else
											{
												temp_timer[requestor] = tWR +tWL+tBUS + max(LPRE[front_has_pre] + tRP + LACT[front_has_act] + tRCD + (tWtoR - tCCD) + N*tCCD + 1 + tRL + tBUS, LPRE[front_has_pre] + 
													tRP + LACT[front_has_act] + tRCD + (tRTW-tCCD) + (bank_count-2)*tCCD + tWtoR +1 + tRL + tBUS);
											}										
										}
										if(commandQueue_RT[requestor]->checkCommand(true,2)->busPacketType == WR)
										{
											if(isReadyTimer(commandQueue_RT[requestor]->getCommand(true),requestor) > tWR)
											{
												temp_timer[requestor] =  isReadyTimer(commandQueue_RT[requestor]->getCommand(true),requestor) + max(LPRE[front_has_pre] + tRP + LACT[front_has_act] + tRCD + (tRTW - tCCD) + N*tCCD + 1 + tWL + tBUS, LPRE[front_has_pre] + 
													tRP + LACT[front_has_act] + tRCD + (tWtoR-tCCD) + (bank_count-2)*tCCD + tRTW + 1 + tWL + tBUS);
											}
											else
											{
												temp_timer[requestor] =   tWR  + tWL + tBUS  + max(LPRE[front_has_pre] + tRP + LACT[front_has_act] + tRCD + (tRTW - tCCD) + N*tCCD + 1 + tWL + tBUS, LPRE[front_has_pre] + 
													tRP + LACT[front_has_act] + tRCD + (tWtoR-tCCD) + (bank_count-2)*tCCD + tRTW + 1 + tWL + tBUS);
											}							
										}
									}
									else
									{
										req_open[requestor] = false;
										if(commandQueue_RT[requestor]->checkCommand(true,2)->busPacketType == RD)
										{		
											temp_timer[requestor] = isReadyTimer(commandQueue_RT[requestor]->getCommand(true),requestor) + max(LPRE[front_has_pre] + tRP + LACT[front_has_act] + tRCD + (tWtoR - tCCD) + N*tCCD + 1 + tRL + tBUS, LPRE[front_has_pre] + 
												tRP + LACT[front_has_act] + tRCD + (tRTW-tCCD) + (bank_count-2)*tCCD + tWtoR +1 + tRL + tBUS);
										}
										else if(commandQueue_RT[requestor]->checkCommand(true,2)->busPacketType == WR)
										{			
											temp_timer[requestor] =  isReadyTimer(commandQueue_RT[requestor]->getCommand(true),requestor) + max(LPRE[front_has_pre] + tRP + LACT[front_has_act] + tRCD + (tRTW - tCCD) + N*tCCD + 1 + tWL + tBUS, LPRE[front_has_pre] + 
												tRP + LACT[front_has_act] + tRCD + (tWtoR-tCCD) + (bank_count-2)*tCCD + tRTW + 1 + tWL + tBUS);
										}	
									}
									
								}
								else
								{
									req_open[requestor] = false;
									if(commandQueue_RT[requestor]->checkCommand(true,2)->busPacketType == RD)
									{						
										// return close read wcl
										temp_timer[requestor] = isReadyTimer(commandQueue_RT[requestor]->getCommand(true),requestor) + max(LPRE[front_has_pre] + tRP + LACT[front_has_act] + tRCD + (tWtoR - tCCD) + N*tCCD + 1 + tRL + tBUS, LPRE[front_has_pre] + 
											tRP + LACT[front_has_act] + tRCD + (tRTW-tCCD) + (bank_count-2)*tCCD + tWtoR +1 + tRL + tBUS);
									}
									else if(commandQueue_RT[requestor]->checkCommand(true,2)->busPacketType == WR)
									{			
										// return close write wcl
										temp_timer[requestor] =  isReadyTimer(commandQueue_RT[requestor]->getCommand(true),requestor) + max(LPRE[front_has_pre] + tRP + LACT[front_has_act] + tRCD + (tRTW - tCCD) + N*tCCD + 1 + tWL + tBUS, LPRE[front_has_pre] + 
											tRP + LACT[front_has_act] + tRCD + (tWtoR-tCCD) + (bank_count-2)*tCCD + tRTW + 1 + tWL + tBUS);
									}							
								}
							}
							req_open[requestor] = false;
							if(commandQueue_RT[requestor]->checkCommand(true,2)->busPacketType == RD)
							{							
								// return close read wcl
								temp_timer[requestor] = isReadyTimer(commandQueue_RT[requestor]->getCommand(true),requestor) + max(LPRE[front_has_pre] + tRP + LACT[front_has_act] + tRCD + (tWtoR - tCCD) + N*tCCD + 1 + tRL + tBUS, LPRE[front_has_pre] + 
									tRP + LACT[front_has_act] + tRCD + (tRTW-tCCD) + (bank_count-2)*tCCD + tWtoR +1 + tRL + tBUS);
							}
							else if(commandQueue_RT[requestor]->checkCommand(true,2)->busPacketType == WR)
							{								
								// return close write wcl
								temp_timer[requestor] =  isReadyTimer(commandQueue_RT[requestor]->getCommand(true),requestor) + max(LPRE[front_has_pre] + tRP + LACT[front_has_act] + tRCD + (tRTW - tCCD) + N*tCCD + 1 + tWL + tBUS, LPRE[front_has_pre] + 
									tRP + LACT[front_has_act] + tRCD + (tWtoR-tCCD) + (bank_count-2)*tCCD + tRTW + 1 + tWL + tBUS);
							}
						}
					}
					else
					{
						temp_timer[requestor] = 0;
					}
					
				}
			return temp_timer[requestor];	
			}
		// ---------------------------------Fnction to Set the Deadline---------------------------------
		void set_deadline(unsigned int num)
		{	
			bool order_fix = false;		
			if(commandQueue_RT[num]->getSize(true) > 0) // Return the buffer size of the requestor
			{
				if(deadline_set_flag[num] == false)
				{
					service_deadline[num] = init_deadline;
					deadline_track[num] = clock;
					deadline_set_flag[num] = true;
					Order.push_back(num);	
				}
				else
				{	
					for(unsigned int h=0; h<Order.size() ; h++)
					{
						if(Order.at(h) == num)
						{
							order_fix = true;							
						}
					}
					if(!order_fix){
						Order.push_back(num);
						order_fix = false;
					}					
				}				
			}
			else
			{
				service_deadline[num] = init_deadline;
			}					
		}
		BusPacket* commandSchedule()
		{
		
			scheduledCommand = NULL;
			tRL  = getTiming("tRL");
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
			
			if(mode == "RTMode" && clock != 1)
			{		
				if(scheduledCommand_RTMode != NULL) {
					scheduledCommand = scheduledCommand_RTMode;
					
					if(scheduledCommand->busPacketType < ACT) {			
						servedFlags[scheduledCommand->bank] = true;					
						req_statues_flag[scheduledCommand->requestorID] = false;
					}
					queuePending[commandRegisters[registerIndex].second] = false;										
					sendCommand(scheduledCommand, commandRegisters[registerIndex].second, false);
					commandRegisters.erase(commandRegisters.begin()+registerIndex);
				}
			}
			else if( mode == "HPMode" &&  clock != 1)
			{
				if(scheduledCommand_HPMode != NULL) 
				{
					scheduledCommand = scheduledCommand_HPMode;
					if(scheduledCommand->busPacketType < ACT) {							
						req_statues_flag[scheduledCommand->requestorID] = false;
					}
					
					sendCommand(scheduledCommand,scheduledCommand->requestorID, false);
				}	
			}

			if(flag_deadline){
				deadline_set_flag[scheduledCommand->requestorID] = false;
				flag_deadline = false;
			}

			if(scheduledCommand != NULL){
				if(scheduledCommand->busPacketType == RD || scheduledCommand->busPacketType == WR)
				{
					//cout<<"there was a CAS issued so the RR order needs to be modified"<<endl;
					for(unsigned int h=0; h<Order.size() ; h++){
						if(Order.at(h) == scheduledCommand->requestorID)
						{
							Order.erase(Order.begin() + h);
							break;
						}
					}
				}
			}
			
			for (unsigned int i = 0; i< bank_count ; i++)
				set_deadline(i);
			
			for (unsigned int i = 0; i< bank_count ; i++)
			{
				if(service_deadline[i] > 0)
					service_deadline[i]--;
				else									
					service_deadline[i] = 0;
			}
		
			//scheduledCommand = NULL;
			checkCommand = NULL;
			checkCommand_temp_1 = NULL;
			checkCommand_temp_2 = NULL;
			scheduledCommand_HPMode= NULL;
			scheduledCommand_RTMode = NULL;
			index_temp = 0;
			
			// now go ahead and schedule the command based on the HPMode controller 
			for(unsigned int index = 0; index < commandQueue.size(); index++)
			{
				if(commandQueue[index]->getSize(true) > 0)
				{
					checkCommand = commandQueue[index]->getCommand(true);
					if(checkCommand != NULL)
					{						
						if(isReady(checkCommand,index))
						{					
							if(isIssuable(checkCommand))
							{
								if(checkCommand_temp_2 != NULL)
								{
									if((checkCommand->busPacketType == RD || checkCommand->busPacketType == WR)){
										
										if(checkCommand->arriveTime < checkCommand_temp_2->arriveTime){
											checkCommand_temp_2 = checkCommand;
											index_temp = index;
										}
											
									}
									else if(checkCommand->busPacketType == ACT){
										if(checkCommand_temp_2->busPacketType == ACT){
											if(checkCommand->arriveTime < checkCommand_temp_2->arriveTime){
												checkCommand_temp_2 = checkCommand;
												index_temp = index;
											}	
										}
										else if(checkCommand_temp_2->busPacketType == PRE){
											checkCommand_temp_2 = checkCommand;
											index_temp = index;
										}
									}	
									else if(checkCommand->busPacketType == PRE){
										if(checkCommand_temp_2->busPacketType == PRE){
											if(checkCommand->arriveTime < checkCommand_temp_2->arriveTime){
												checkCommand_temp_2 = checkCommand;
												index_temp = index;
											}
										}		
									}
									
								}
								else
								{
									checkCommand_temp_2 = checkCommand;
									index_temp = index;
								}																													
							}									
						}
					}	
					checkCommand = NULL;	
				}
				if(index == commandQueue.size()-1){
					if(checkCommand_temp_2 != NULL){
						scheduledCommand_HPMode = checkCommand_temp_2;
						checkCommand_temp_2 = NULL;
					}			
				}
			}
			// Decision needs to be made now. 
			mode_prev = mode;
			if(mode == "HPMode") // if HPMode
			{
				for (unsigned int index = 0; index < bank_count; index++)
				{
					if(actual_wcl(index,scheduledCommand_HPMode) >= service_deadline[index])
					{
						suspect_requestor = index;
						suspect_flag = true;
						mode = "RTMode";
						break;					
					}
				}
			}
			else // if RTMode
			{
				for (unsigned int index = 0; index < bank_count; index++)
				{
					if(actual_wcl(index,scheduledCommand_HPMode) < service_deadline[index])
					{
						
						mode = "HPMode";		
					}
					else
					{
						mode = "RTMode";
						break;
					}					
				}
			}
			
			scheduledCommand_RTMode = NULL;
			if(mode == "RTMode")
			{
				for(unsigned int index = 0; index < commandQueue.size(); index++) { 			// Scan the command queue for ready CAS
					checkCommand = NULL;
					if(commandQueue_RT[index]->getSize(true) > 0 && queuePending[index] == false) {
						checkCommand = commandQueue_RT[index]->getCommand(true);
					}
					if(checkCommand != NULL) {
						if(isReady(checkCommand, index)) {						
							commandRegisters.push_back(make_pair(checkCommand,checkCommand->bank));						
							queuePending[index] = true;	
						}
					}
				}

				bool CAS_remain = false;
				bool newRound = true;	// Should reset servedFlag?
				bool switch_expected = false;	// A different CAS can be schedule
				
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
						if(switch_expected == true){
							newRound = true;		
						}			
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

				registerIndex = 0;
				if(!commandRegisters.empty()) {
					for(unsigned int loop = 0; loop < Order.size(); loop++)
					{
						if(scheduledCommand_RTMode == NULL)
						{
							for(unsigned int index = 0; index < commandRegisters.size(); index++) {
								checkCommand = commandRegisters[index].first;
								if(servedFlags[checkCommand->bank] == false) 
								{
									if(checkCommand->busPacketType == roundType)
									{
										if(checkCommand->requestorID == Order.at(loop))
										{
											if(isIssuable(checkCommand)) {
												scheduledCommand_RTMode = checkCommand;
												
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
				// Round-Robin ACT arbitration
				if(scheduledCommand_RTMode == NULL && !commandRegisters.empty()) {
					for(unsigned int loop = 0; loop < Order.size(); loop++)
					{
						if(scheduledCommand_RTMode == NULL)
						{
							for(unsigned int index = 0; index < commandRegisters.size(); index++) {
								if(commandRegisters[index].first->busPacketType == ACT) {
									checkCommand = commandRegisters[index].first;
									if(isIssuable(checkCommand) && checkCommand->requestorID == Order.at(loop)) {
										scheduledCommand_RTMode = checkCommand;
										
										registerIndex = index;
										break;
									}
									checkCommand = NULL;
								}
							}
						}
					}
				}
				// Round-Robin PRE arbitration
				if(scheduledCommand_RTMode == NULL && !commandRegisters.empty()) {
					for(unsigned int loop = 0; loop < Order.size(); loop++)
					{
						if(scheduledCommand_RTMode == NULL)
						{
							for(unsigned int index = 0; index < commandRegisters.size(); index++) {
								if(commandRegisters[index].first->busPacketType == PRE) {
									checkCommand = commandRegisters[index].first;
									if(isIssuable(checkCommand)) {
										if(checkCommand->requestorID == Order.at(loop))
										{
											scheduledCommand_RTMode = checkCommand;
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
			}	
			// remove anything remained in the RT command registers
			if(mode_prev == "RTMode" && mode == "HPMode"){
				commandRegisters.clear();	
				for (unsigned int y=0; y<commandQueue.size();y++)
				{
					queuePending[y] = false;	
				}
				
			}
			// remove anything remained in the RT command registers
			if(mode_prev == "HPMode" && mode == "RTMode" && suspect_flag){
				
				
					if(commandQueue_RT[suspect_requestor]->getSize(true) > 0  && commandQueue[suspect_requestor]->getSize(true))
					{
						if(commandQueue[suspect_requestor]->getCommand(true)->busPacketType != commandQueue_RT[suspect_requestor]->getCommand(true)->busPacketType)
						{
							// first remove the HP queue
							if(commandQueue[suspect_requestor]->getSize(true)>0)
							{
								unsigned int temp_size = commandQueue[suspect_requestor]->getSize(true);
								for(unsigned int l=0; l < temp_size; l++)
								{
									commandQueue[suspect_requestor]->removeCommand();
								}
							}	
							
							for(unsigned int l=0; l < commandQueue_RT[suspect_requestor]->getSize(true); l++)
							{
								BusPacket* checkCommand_copy;		
								checkCommand_copy = NULL;
								checkCommand_copy = commandQueue_RT[suspect_requestor]->checkCommand(true,l);
								commandQueue[suspect_requestor]->insertCommand(checkCommand_copy,true,false);
							}
							
						}
					}		
				
			requestQueues_local[0]->syncRequest(suspect_requestor,0);
			scheduledCommand_HPMode = NULL;
			suspect_flag = false;
				
			}

			if(mode_prev == "HPMode" && mode == "RTMode")
			{
				if(scheduledCommand_RTMode != NULL)
				{
					if(scheduledCommand_RTMode->busPacketType == PRE)
					{
						if(commandQueue[scheduledCommand_RTMode->requestorID]->getSize(true)>0)
						{
						
							
							unsigned int temp_size = commandQueue[scheduledCommand_RTMode->requestorID]->getSize(true);
							
							for(unsigned int l=0; l < temp_size; l++)
							{
								commandQueue[scheduledCommand_RTMode->requestorID]->removeCommand();
							}
						}
						
						for(unsigned int l=0; l < commandQueue_RT[scheduledCommand_RTMode->requestorID]->getSize(true); l++)
						{
							BusPacket* checkCommand_copy;		
							checkCommand_copy = NULL;
							checkCommand_copy = commandQueue_RT[scheduledCommand_RTMode->requestorID]->checkCommand(true,l);
							commandQueue[scheduledCommand_RTMode->requestorID]->insertCommand(checkCommand_copy,true,false);
						}
						
						requestQueues_local[0]->syncRequest(scheduledCommand_RTMode->requestorID,0);
					}
				}

			}
			if(mode_prev == "RTMode" && mode == "RTMode")
			{
				if(scheduledCommand_RTMode != NULL)
				{
					if(scheduledCommand_RTMode->busPacketType == PRE)
					{
						if(commandQueue[scheduledCommand_RTMode->requestorID]->getSize(true)>0)
						{
							unsigned int temp_size = commandQueue[scheduledCommand_RTMode->requestorID]->getSize(true);
						
							for(unsigned int l=0; l < temp_size; l++)
							{
								commandQueue[scheduledCommand_RTMode->requestorID]->removeCommand();
							}
						}
						
						for(unsigned int l=0; l < commandQueue_RT[scheduledCommand_RTMode->requestorID]->getSize(true); l++)
						{
							BusPacket* checkCommand_copy;		
							checkCommand_copy = NULL;
							checkCommand_copy = commandQueue_RT[scheduledCommand_RTMode->requestorID]->checkCommand(true,l);
							commandQueue[scheduledCommand_RTMode->requestorID]->insertCommand(checkCommand_copy,true,false);
						}
						
						requestQueues_local[0]->syncRequest(scheduledCommand_RTMode->requestorID,0);
					}
				}
			}
			if(mode_prev == "RTMode" && mode == "HPMode")
			{				
				for(unsigned int index = 0; index < servedFlags.size(); index++) {
					servedFlags[index] = false;
				}
				roundType = BusPacketType::RD;	// Read Typ
			}
			if(mode == "HPMode"){
			
				if(scheduledCommand_HPMode != NULL)
				{
					if(scheduledCommand_HPMode->busPacketType == RD || scheduledCommand_HPMode->busPacketType == WR){
				
						requestQueues_local[0]->updateRowTable(scheduledCommand_HPMode->rank, scheduledCommand_HPMode->bank, scheduledCommand_HPMode->row);
					
						if(scheduledCommand_HPMode->address == commandQueue_RT[scheduledCommand_HPMode->requestorID]->getCommand(true)->address){
							if(scheduledCommand_HPMode->busPacketType == commandQueue_RT[scheduledCommand_HPMode->requestorID]->getCommand(true)->busPacketType){
								flag_deadline = true;
							}
						}

						commandQueue[scheduledCommand_HPMode->requestorID]->removeCommand();
						if(commandQueue_RT[scheduledCommand_HPMode->requestorID]->getSize(true)>0)
						{							
							if(scheduledCommand_HPMode->address == commandQueue_RT[scheduledCommand_HPMode->requestorID]->getCommand(true)->address && scheduledCommand_HPMode->busPacketType == commandQueue_RT[scheduledCommand_HPMode->requestorID]->getCommand(true)->busPacketType ){							
								
								if(clock - deadline_track[scheduledCommand_HPMode->requestorID] + tRL + 4 + 1 > 150)
									abort();
								commandQueue_RT[scheduledCommand_HPMode->requestorID]->removeCommand();
							}
							
						}
						
						requestQueues_local[0]->removeRequest(false,scheduledCommand_HPMode->requestorID);
					}
				}
			}	
			else
			{
				
				if(scheduledCommand_RTMode != NULL)
				{
					
					if(scheduledCommand_RTMode->busPacketType == RD || scheduledCommand_RTMode->busPacketType == WR){
					
						if(clock - deadline_track[scheduledCommand_RTMode->requestorID] + tRL + 4 + 1 > 150)
							abort();
						flag_deadline = true;
						requestQueues_local[0]->updateRowTable(scheduledCommand_RTMode->rank, scheduledCommand_RTMode->bank, scheduledCommand_RTMode->row);
				
						if(commandQueue[scheduledCommand_RTMode->requestorID]->getSize(true)>0)
						{
							if(scheduledCommand_RTMode->address == commandQueue[scheduledCommand_RTMode->requestorID]->getCommand(true)->address && scheduledCommand_RTMode->busPacketType ==commandQueue[scheduledCommand_RTMode->requestorID]->getCommand(true)->busPacketType )							
								commandQueue[scheduledCommand_RTMode->requestorID]->removeCommand();
						}
						commandQueue_RT[scheduledCommand_RTMode->requestorID]->removeCommand();
						
						requestQueues_local[0]->removeRequest(true,scheduledCommand_RTMode->requestorID);
					}
				}
			}	
			return scheduledCommand;
		}
	};
}
#endif /* COMMANDSCHEDULER_FRFCFS_H */

