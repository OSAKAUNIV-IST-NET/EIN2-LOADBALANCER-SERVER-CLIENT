#include <gtest/gtest.h>
#include <stdexcept>
#include "client.h"

class ClientTest : public testing::Test
{
public:
  ClientTest() {};
  virtual ~ClientTest() {};
protected:
  virtual void SetUp()
  {
    Config *c = Config::GetInstance();
    c->load("../examples/localhost.conf");
  }
};

// Initialize
TEST_F(ClientTest, initialize)
{
  config_t c = Config::GetInstance()->getConfig();
  Client *client = new Client(c.client[0], c.server[0]);

  EXPECT_STREQ("127.0.0.1", client->ip_addr);
  ASSERT_EQ(1, client->srv_array.size());
  EXPECT_EQ("127.0.0.1", client->srv_array[0]->ip_addr);
  EXPECT_EQ(12345, client->srv_array[0]->port);

  delete client;
}
