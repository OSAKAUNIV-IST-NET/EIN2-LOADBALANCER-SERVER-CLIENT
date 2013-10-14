#include <iostream>
#include <unistd.h>
#include <stdlib.h>
#include <string>
#include <fstream>
#include <sys/wait.h>
#include <errno.h>
#include <vector>
using namespace std;

#include "common.h"
#include "picojson.h"
using namespace picojson;

/*
 * Parse Option
 */
int parse_option(int argc, char *argv[])
{
  int result;

  while((result = getopt(argc, argv, "h")) != -1){
    switch(result){
    case 'h':
      return -1;
      break;
    case '?':
      return -1;
      break;
    }
  }

  return 0;
}

/**
 * Wait for all process finished
 */
void wait_all(const char *name = "Process")
{
  pid_t pid;
  int status;
  bool results = true;
  char err_str[TMP_BUF_SIZE] = "";
				
  while(1){
    pid = wait(&status);
    
    if(pid == -1){
      // child process does not exist.
      if(ECHILD == errno) break;
      else if(EINTR == errno) continue;
      strcat(err_str, name);
      strcat(err_str, ": wait");
      perror(err_str);
      exit(EXIT_FAILURE);
    }
    if(status != 0) results = false;
    // printf("parent: child = %d, status=%d\n", pid, status);
  }

  if(!results) exit(EXIT_FAILURE);
}

/**
 * Constructor
 */
Config::Config()
{
  /* initialize 'config' */
  config.num_pkts = DEFAULT_NUM_PKTS;
  config.duration = 0;
}

/**
 * Deconstructor
 */
Config::~Config()
{
  /* delete 'config' values */
  free_config();
}

/**
 * Free dynamically allocated values of 'config'
 */
void Config::free_config()
{
  /* for client */
  config.client.clear();
  /* for server */
  config.server.clear();
}

/*
 * Clear config
 */
void Config::clear()
{
  free_config();

  config.num_pkts = DEFAULT_NUM_PKTS;
  config.duration = 0;
}

/*
 * Read file
 */
void Config::read_file(const char *fn, string *str)
{
  ifstream ifs(fn);
  string line;
  *str = "";

  /* open file */
  if(ifs.fail()){
    perror("Cannot open config file");
    exit(EXIT_FAILURE);
  }

  /* copy strings of the config */
  while(getline(ifs, line)){
    *str = *str + line;
  }
}

/*
 * Parse Config
 */
config_t Config::parse_config(const char *str)
{
  /* parse config with 'picojson' parser */
  value v;

  parse(v, str, str + strlen(str));

  if(v.is<object>()){
    // global parameter
    value n_pkts = v.get("n_pkts");
    if(n_pkts.is<double>()){
      config.num_pkts = n_pkts.get<double>();
    }

    value duration = v.get("duration");
    if(duration.is<double>()){
      config.duration = duration.get<double>();
    }

    // for client
    value clients = v.get("client");
    if(clients.is<array>()){
      array a = clients.get<array>();
      for(int i=0; i < a.size(); i++){
	client_t *c = new client_t;
	if(a[i].get("ip").is<string>()){
	  c->ip_addr = a[i].get("ip").get<string>();
	} else {
	  c->ip_addr = "";
	}
	if (a[i].get("ack_timeout").is<double>()){
	  c->ack_timeout = (int)a[i].get("ack_timeout").get<double>();
	}
	config.client.push_back(c);
      }
    }

    // for server
    value servers = v.get("server");
    if(servers.is<array>()){
      array a = servers.get<array>();
      for(int i=0; i< a.size(); i++){
	server_t *s = new server_t;
	if(a[i].get("ip").is<string>()){
	  s->ip_addr = a[i].get("ip").get<string>();
	} else {
	  s->ip_addr = "";
	}
	if(a[i].get("port").is<double>()){
	  s->port = (int)a[i].get("port").get<double>();
	}
	if(a[i].get("max_life").is<double>()){
	  s->max_life = (int)a[i].get("max_life").get<double>();
	}
	if(a[i].get("dec_life").is<double>()){
	  s->dec_life = (int)a[i].get("dec_life").get<double>();
	}
	if(a[i].get("sleep_time").is<double>()){
	  s->sleep_time = (int)a[i].get("sleep_time").get<double>();
	}
	config.server.push_back(s);
      }
    }
  }

  return config;
}

/**
 * Load Config File
 */
void Config::load(const char *fn)
{

  string str;

  /* read config file */
  read_file(fn, &str);

  /* clear config */
  clear();
  /* parse string and get config */
  config = parse_config(str.c_str());
}

/**
 * Check the loaded config is whether valid or not.
 */
bool Config::is_valid_config()
{
  /* check 'num_pkts' */
  if(config.num_pkts < 0) return false;

  /* check 'duration' */
  if(config.duration < 0) return false;

  /* check 'client' */
  vector<client_t *> c = config.client;
  if(c.size() == 0) return false;
  for(int i=0; i < c.size(); i++){
    if(c[i]->ip_addr == "") return false;
    if(!(c[i]->ack_timeout > 0)) return false;
  }


  /* check 'server' */
  vector<server_t *> s = config.server;
  if(s.size() == 0) return false;
  for(int i=0; i < s.size(); i++){
    if(s[i]->ip_addr == "") return false;
    if(!(s[i]->port > 0 && s[i]->port < 65536)) return false;    
    if(!(s[i]->max_life > 0)) return false;
    if(s[i]->dec_life < 0) return false;
    if(s[i]->sleep_time < 0) return false;
  }


  return true;
}
