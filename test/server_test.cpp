#include <gtest/gtest.h>
#include <stdexcept>

#include "server.h"
#include "common.h"

class ServerTest : public testing::Test
{
public:
  ServerTest() {};
  virtual ~ServerTest() {};
protected:
  virtual void SetUp()
  {
    Config *c = Config::GetInstance();
    c->load("../examples/localhost.conf");
  }
};

// Initialize
TEST_F(ServerTest, initialize)
{
  config_t c = Config::GetInstance()->getConfig();
  Server *s = new Server(c.server[0]);
  EXPECT_EQ(0, s->getNumRecvPkts());
  EXPECT_EQ(100, s->getLife());
}

// IPC_Addr
TEST_F(ServerTest, IPC_Addr)
{
  const char *ip_addr = "192.168.2.123";
  int port = 23456;
  EXPECT_STREQ("/tmp/ofg192.168.2.123:23456",
	       Server::IPC_Addr(ip_addr, port));
}

