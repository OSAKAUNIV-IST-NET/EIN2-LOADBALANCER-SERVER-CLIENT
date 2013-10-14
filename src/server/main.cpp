#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <vector>
#include <pthread.h>
#include <dirent.h>

#include "common.h"
#include "server.h"


/**
 * main
 */
int main(int argc, char *argv[])
{
  // const char *usage = "Usage: server config_path";
  const char *usage = "Usage: server ip port max_life dec_life sleep_time";

  /* check number of args */
  if(argc != 6){
    fprintf(stderr, "%s\n", usage);
    exit(EXIT_FAILURE);
  }

  /* Get name of interface "trema*" */
  char iface_name[10];
  DIR *dp = opendir("/sys/class/net/");
  if (dp!=NULL) {
    struct dirent* dent;
    dent = readdir(dp);
    while(dent!=NULL) {
      if (dent!=NULL) {
	if (dent->d_name[0] == 't') {
	  strncpy(iface_name, dent->d_name, sizeof(iface_name));
	}
      }
      dent = readdir(dp);
    }
    closedir(dp);
  } else {
    cerr << "ERROE: opendir" << endl;
    exit(1);
  }

  /* Get MAC address for my interface */
  string f_name = "/sys/class/net/" + string(iface_name) + "/address";
  FILE *fp = fopen(f_name.c_str(), "r");
  char mac_address[18];
  if (fp != NULL) {
    fgets(mac_address, sizeof(mac_address), fp);
    fclose(fp);
  } else {
    cerr << "ERROE: mac_address" << endl;
    exit(1);
  }

  /* Open life point file */
  string filename = string(LIFE_FILE_DIR) + string(LIFE_FILE_PREFIX) + string("_") + string(argv[1]) + string("_") + string(mac_address);
  FILE *life_file;
  if ((life_file = fopen(filename.c_str(), "w")) == NULL) {
    cerr << "ERROE: fopen" << endl;
    fclose(life_file);
    exit(1);
  }

  pthread_t srv_th;
  
  // Run server
  /* Server(const char *ip_addr, int port, int port_ack, int max_life, int dec_life, int sleep_time, FILE *file) */
  Server *srv = new Server(argv[1], atoi(argv[2]), atoi(argv[2]), atoi(argv[3]), atoi(argv[4]), atoi(argv[5]), life_file);
  srv->life_filename = filename;
  /* Set name of my interface */
  for (int i = 0; i < sizeof(iface_name); i++) {
    srv->iface_name[i] = iface_name[i];
  }
  /* Set MAC address */
  for (int i = 0; i < sizeof(mac_address); i++) {
    srv->mac_address[i] = mac_address[i];
  }
  pthread_create(&srv_th, NULL, Server::Run, reinterpret_cast<void *>(srv));

  char buf[TMP_BUF_SIZE];
  while(true){
    //while(scanf("%s", buf)==EOF);
    scanf("%s", buf);
    if("fin" == string(buf)) break;
  }


  srv->show_score();
  delete srv;

  return 0;
}
