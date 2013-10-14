#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <pthread.h>

#include "common.h"
#include "client.h"

/*
 * main
 */
int main(int argc, char *argv[])
{
  const char *usage = "Usage: client ip config_path";

  /* check number of args */
  if(argc != 3){
    fprintf(stderr, "%s\n", usage);
    exit(-1);
  }

  /* get config */
  Config *conf = Config::GetInstance();
  conf->load(argv[2]);
  if(!conf->is_valid_config()){
    fprintf(stderr, "Config file is wrong.\n");
    fprintf(stderr, "%s\n", usage);
    exit(-1);
  }
  config_t d_conf = conf->getConfig();

  /* Create clients*/
  client_t *clt = new client_t;
  vector<client_t *>clt_vec = d_conf.client;
  for (int i=0; i<clt_vec.size(); i++) {
    if (clt_vec[i]->ip_addr == string(argv[1])) {
      clt->ip_addr = string(argv[1]);
      clt->ack_timeout = clt_vec[i]->ack_timeout;
      break;
    }
  }
  vector<server_t *>srv = d_conf.server;
  vector<Client *> clt_list;
  //  for(int i=0; i<srv.size(); i++){
  for(int i=0; i==0; i++){
    Client *client = new Client(clt, srv[i]);
    clt_list.push_back(client);
  }

  /* run clients */
  vector<pthread_t *> thr_grp;
  for(int i=0; i<clt_list.size(); i++){
    pthread_t *thr = new pthread_t;
    thr_grp.push_back(thr);
    pthread_create(thr, NULL, Client::Run, reinterpret_cast<void*>(clt_list[i]));
  }
  for(int i=0; i<thr_grp.size(); i++){
    pthread_join(*thr_grp[i], NULL);
  }

  /* Show Score*/
  // Client::Show(clt_list);

  clt_list.clear();
  delete clt;

  return 0;
}
