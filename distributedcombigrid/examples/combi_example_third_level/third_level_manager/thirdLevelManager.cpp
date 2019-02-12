#include "thirdLevelManager.hpp"

int main(int argc, char* argv[])
{
  // Load config file
  std::cout << "Loading parameters" << std::endl;
  Params params;
  params.loadFromFile("example.ini");

  // Run third level manager
  std::cout << "Setup third level manager" << std::endl;
  ThirdLevelManager manager(params);

  std::cout << std::endl;
  std::cout << "Running third level manager" << std::endl;
  manager.runtimeLoop();
  return 0;
}

/*
 * Initiates the third level manager by reading in parameters and creating
 * abstractions for each managed system.
 */
ThirdLevelManager::ThirdLevelManager(const Params& params)
  : params_(params),
    dataPort_(params.getDataPort()),
    dataServer_(dataPort_)
{
  // Connect to RabbitMQ Broker
  std::cout << "Connecting to RabbitMQ broker" << std::endl;
  channel_ = AmqpClient::Channel::Create(params.getbrokerURL());


  // Create abstraction of each system and establish message queues and data channel
  std::cout << "Creating abstraction for systems" << std::endl;
  std::vector<std::string> systemNames = params.getSystemNames();
  systems_.reserve(systemNames.size());
  for (auto nameIt = systemNames.begin(); nameIt != systemNames.end(); nameIt++)
  {
    systems_.push_back(System(*nameIt, channel_, dataServer_));
    std::cout << "  added system: " << *nameIt << std::endl;
  }
}

/*
 * Loops over the systems and checks if messages are available. If a message 
 * exists, it is fetched and deligated to be processed.
 */
void ThirdLevelManager::runtimeLoop()
{
  while (systems_.size() > 0)
  {
    for (auto sysIt = systems_.begin(); sysIt != systems_.end(); sysIt++)
    {
      std::string message;
      bool received = sysIt->receiveMessage(channel_, message, timeout_);
      if (received)
        processMessage(message, *sysIt);
    }
  }
}

/*
 * Identifies message and initiates appropriate action.
 */
void ThirdLevelManager::processMessage(const std::string& message, System& system)
{
  if (message == "ready_to_combine")
    processCombination(system);
  if (message == "finished_computation")
    processFinished(system);
}

/*
 * Processes and manges the third level combination, initiated by a system
 * which signals ready after local and global combination.
 */
void ThirdLevelManager::processCombination(System& system)
{
  std::cout << "Processing third level combination" << std::endl;

  std::string message = "send_size";
  system.sendMessage(message, channel_);

  system.receiveMessage(channel_, message);
  assert(NetworkUtils::isInteger(message));
  size_t transferSize = std::stoi(message);

  message = "combine_third_level_send_first";
  system.sendMessage(message, channel_);

  for (auto sysIt = systems_.begin(); sysIt != systems_.end(); sysIt++)
  {
    if (sysIt->getName() != system.getName())
    {
      sysIt->receiveMessage(channel_, message, timeout_);
      assert(message =="ready_to_combine");
      system.sendMessage("send_size", channel_);
      size_t expectedSize = std::stoi(message);
      assert(expectedSize == transferSize);
      sysIt->sendMessage("combine_third_level_recv_first", channel_);

      // establish exchange of locally combinated subspaces.
      std::cout << "Forwarding " << transferSize << " Bytes between systems: " << system.getName()  << " and " << sysIt->getName() << std::endl;
      NetworkUtils::forward(system.getDataConnection().get(),
          sysIt->getDataConnection().get(), transferSize);

      NetworkUtils::forward(sysIt->getDataConnection().get(),
          system.getDataConnection().get(), transferSize);
    }
  }
}

/*
 * If a system has finished the simulation, it should log of from the
 * third level manager
 */
void ThirdLevelManager::processFinished(System& system)
{
  for (auto sysIt = systems_.begin(); sysIt != systems_.end(); sysIt++)
  {
    if (sysIt->getName() == system.getName())
      systems_.erase(sysIt);
  }
}