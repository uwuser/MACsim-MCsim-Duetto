#include "global.h"
#include "CommandGenerator.h"

#include <algorithm>  

using namespace MCsim;
using namespace std;

CommandGenerator::CommandGenerator(unsigned int dataBus,std::vector<CommandQueue*>& commandQueues, std::vector<CommandQueue*>& commandQueues_RT):
	dataBusSize(dataBus),
	commandQueue(commandQueues),
	commandQueue_RT(commandQueues_RT)

{}

CommandGenerator::~CommandGenerator()
{
	lookupTable.clear();
	std::queue<BusPacket*> empty;
	std::swap(commandBuffer, empty);
}
// Return the head cmd from the general cmd buffer
BusPacket* CommandGenerator::getCommand() 
{
	return commandBuffer.front();
}
// Pop the cmd from the head of the general cmd buffer
void CommandGenerator::removeCommand()
{
	commandBuffer.pop();
}
// Return the general cmd buffer size
unsigned CommandGenerator::bufferSize() 
{
	return commandBuffer.size();
}
// Return the general cmd buffer size
unsigned CommandGenerator::bufferSize_RT() 
{
	return commandBuffer_RT.size();
}
// Pop the cmd from the head of the general cmd buffer
void CommandGenerator::removeCommand_RT()
{
	commandBuffer_RT.pop();
}
// Return the head cmd from the general cmd buffer
BusPacket* CommandGenerator::getCommand_RT() 
{
	return commandBuffer_RT.front();
}