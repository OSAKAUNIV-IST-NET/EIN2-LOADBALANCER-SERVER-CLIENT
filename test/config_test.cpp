#include <gtest/gtest.h>
#include <stdexcept>
#include <vector>
using namespace std;

#include "common.h"


// Initialize
TEST(ConfigTest,initialize)
{
  Config *conf = Config::GetInstance();
}

// Parse config
TEST(ConfigTest, parse_config)
{
  Config *c = Config::GetInstance();

  config_t conf;
  c->clear();

  /* -------  normal test ------- */
  const char *str = "{\n \"n_pkts\" : 100,\n \"client\" : [ {\"ip\" : \"192.168.1.1\"}], \"server\" : [{\"ip\":\"192.168.1.100\", \"port\" : 12345}]}\n";

  conf = c->parse_config(str);

  /* for global parameters */
  EXPECT_EQ(100, conf.num_pkts);
  /* for client */
  EXPECT_EQ(1, conf.client.size());
  EXPECT_EQ("192.168.1.1", conf.client[0]->ip_addr);
  /* for server */
  EXPECT_EQ(1, conf.server.size());
  EXPECT_EQ("192.168.1.100", conf.server[0]->ip_addr);
  EXPECT_EQ(12345, conf.server[0]->port);
  /* ------- normal test end ------- */

  /* Exception 1:  */
}


TEST(ConfigTest, load)
{
  Config *conf = Config::GetInstance();
  config_t c;

  conf->clear();

  conf->load("../examples/network.conf");

  c = conf->getConfig();

  EXPECT_EQ(1000, c.num_pkts);

  EXPECT_EQ(2, c.client.size());
  EXPECT_EQ("192.168.1.1", c.client[0]->ip_addr);
  EXPECT_EQ("192.168.1.5", c.client[1]->ip_addr);

  EXPECT_EQ(3, c.server.size());
  EXPECT_EQ("192.168.1.100", c.server[0]->ip_addr);
  EXPECT_EQ(12345, c.server[0]->port);
  EXPECT_EQ("192.168.1.102", c.server[1]->ip_addr);
  EXPECT_EQ(22345, c.server[1]->port);
  EXPECT_EQ("192.168.1.103", c.server[2]->ip_addr);
  EXPECT_EQ(32345, c.server[2]->port);
}

TEST(ConfigTest, is_valid_config)
{
  Config *c = Config::GetInstance();
  c->clear();

  /* valid config */
  c->config.num_pkts = 10000;
  vector<client_t *> clt;
  client_t *client = new client_t;
  client->ip_addr = "192.168.1.11";
  clt.push_back(client);
  c->config.client = clt;

  vector<server_t *> srv;
  server_t *server = new server_t;
  server->ip_addr = "192.168.2.22";
  server->port = 10222;
  srv.push_back(server);
  c->config.server = srv;

  EXPECT_TRUE(c->is_valid_config());

  /* invalid config */
  /** invalid num_pkts **/
  c->config.num_pkts = 0;
  EXPECT_FALSE(c->is_valid_config());
  /** invalid client's ip **/
  c->config.num_pkts = 1000;
  c->config.client[0]->ip_addr = "";
  EXPECT_FALSE(c->is_valid_config());
  /** invalid client **/
  c->config.client.clear();
  EXPECT_FALSE(c->is_valid_config());
}
