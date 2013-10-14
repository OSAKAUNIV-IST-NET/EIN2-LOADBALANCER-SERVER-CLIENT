#ifndef __COMMON_H__
#define __COMMON_H__

/*
 * Header file for common library
 */

#include <iostream>
#include <string>
#include <vector>

using namespace std;

#include "gtest/gtest_prod.h"

#define DEFAULT_NUM_PKTS 1000
#define TMP_BUF_SIZE  256

typedef enum { RUN, FIN } ofg_state;

struct client_t {
  string ip_addr;
  int ack_timeout;
};


struct server_t {
  string ip_addr;
  int port;
  int max_life;
  int dec_life;
  int sleep_time;
};

struct config_t {
  vector<client_t *> client;
  vector<server_t *> server;

  int num_pkts;
  int duration;
};


int parse_option(int argc, char *argv[]);
void wait_all(const char *name);


/*
 * Config Class
 */
class Config
{
 private:
  FRIEND_TEST(ConfigTest, parse_config);
  FRIEND_TEST(ConfigTest, is_valid_config);

  config_t config;

  Config();
  ~Config();
  void read_file(const char *fn, string *str);
  void free_config();
  config_t parse_config(const char *str);

 public:

  config_t getConfig() { return config; };
  void load(const char *fn);
  void clear();
  bool is_valid_config();
  static Config* GetInstance()
  {
    static Config config;

    return &config;
  }
};


#endif /* __COMMON_H__ */
