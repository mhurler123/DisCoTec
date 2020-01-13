#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
#include <mpi.h>
#include "sgpp/distributedcombigrid/third_level/NetworkUtils.hpp"
#include "test_helper.hpp"

static std::string host             = "localhost";
static unsigned short port          = 9999;
static std::vector<double> testData = std::vector<double>({0., 1., 2., 3., 4.});


void testBinarySendClient() {
  // wait until server is set up
  MPI_Barrier(MPI_COMM_WORLD);

  // connect to server
  ClientSocket client(host, port);
  client.init();

  // send data
  client.sendallBinary(testData.data(), testData.size());
}

void testBinarySendRecvServer() {
  // setup server
  ServerSocket server(port);
  server.init();

  // server is now ready to accept client
  MPI_Barrier(MPI_COMM_WORLD);
  std::shared_ptr<ClientSocket> client(server.acceptClient());
  BOOST_CHECK(client != nullptr);

  // receive data
  std::vector<double> recvData(testData.size());
  client->recvallBinaryAndCorrectInPlace(recvData.data(), recvData.size());

  // check if received data has been successfully received
  std::cout << std::endl << "-- Receive Test:" << std::endl;
  for (size_t i = 0; i < recvData.size(); ++i) {
    std::cout << "received: " << recvData[i] << " expected: " << testData[i] << std::endl;
    BOOST_CHECK(recvData[i] == testData[i]);
  }
}

void testBinarySendReduceServer() {
  // init server
  ServerSocket server(port);
  server.init();

  // server is now ready to accept client
  MPI_Barrier(MPI_COMM_WORLD);
  std::shared_ptr<ClientSocket> client(server.acceptClient());

  // reduce data
  std::vector<double> recvData(testData);
  client->recvallBinaryAndReduceInPlace<double>(recvData.data(),
      recvData.size(),
      [](const double& a, const double& b) -> double {return a + b;});

  // check if received data has been successfully reduced
  std::cout << std::endl << "-- Reduce Test:" << std::endl;
  for (size_t i = 0; i < recvData.size(); ++i) {
    std::cout << "received: " << recvData[i] << " expected: " << 2*testData[i] << std::endl;
    BOOST_CHECK(recvData[i] == 2*testData[i]);
  }
}


BOOST_AUTO_TEST_SUITE(networkUtils)

BOOST_AUTO_TEST_CASE(testBinarySendRecv) {
  BOOST_REQUIRE(TestHelper::checkNumMPIProcsAvailable(2));
  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  if (rank == 0)
    testBinarySendRecvServer();
  if (rank == 1)
    testBinarySendClient();
}

BOOST_AUTO_TEST_CASE(testBinarySendReduce) {
  BOOST_REQUIRE(TestHelper::checkNumMPIProcsAvailable(2));
  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  if (rank == 0)
    testBinarySendReduceServer();
  if (rank == 1)
    testBinarySendClient();
}

BOOST_AUTO_TEST_SUITE_END()