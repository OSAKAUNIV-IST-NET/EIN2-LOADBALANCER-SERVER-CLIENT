/**
 * Header file for Server
 */

#ifndef __SERVER_H__
#define __SERVER_H__

#include <string>
#include <sstream>
#include <vector>
#include <sys/socket.h>
#include <sys/un.h>

#include "common.h"

#include <time.h>
#include <signal.h>
#include <sys/time.h>
#include <stdint.h>

#define RECV_BUFF 2048
#define IPC_NAME "/tmp/ofg"
#define LIFE_FILE_DIR "/tmp/"
#define LIFE_FILE_PREFIX "server"
//#define DEBUG
//#define VERBOSE

class Server
{
 private:

  FILE *life_file;

  char ip_addr[20];
  uint32_t my_ipv4_address;

  int port;
  int port_ack;

  uint32_t n_rcv_pkts;
  int max_life;
  int life;
  double score;
  int decrement_life;
  int sleep_time;

  void setup_udp_server(int *sock, struct sockaddr_in *addr);
  void setup_ipc_server(int *sock, struct sockaddr_un *addr);
  int accept_ipc_server(int sock, struct sockaddr_un addr);

  bool output_life(int life, FILE *life_file);
  bool output_life(int life);
  uint32_t ipv4_char_to_uint32(char *ip_addr);

  /* 
   * static variables below are used with signal handling
   * ### MUST BE MODIFIED ###
   * (Sorry, I'm not familiar with signal handling with C++)
   */
  static FILE *life_file_;
  static bool is_active;
  static int max_life_;
  static void sigalrm_handler(int sig);
  //

 protected:

  void recv_udp_packets();
  void recv_packets(int sock, struct sockaddr_in addr);
  ofg_state proc_ipc(int sock);
  void send_msg_info(int sock);

 public:
  /* main() */
  char mac_address[18];
  char iface_name[10];
  string life_filename;
  //

  Server(const char *ip_addr, int port, int port_ack, int max_life, int dec_life, int sleep_time, FILE *file);
  Server(server_t *server);   /* used only for test script */
  ~Server();

  unsigned int getNumRecvPkts() {return n_rcv_pkts;};
  double getLife() {return life;};
  double getScore() {return score;};
  void run();  
  void show_score();

  static const char *IPC_Addr(const char *ip_addr, int port)
  {
    string addr = IPC_NAME + string(ip_addr);
    stringstream out;
    addr = addr + ":";
    out << port;
    addr = addr + out.str();
    
    return addr.c_str();
  };
  //  static void Score(vector<Server *> srv_list);
  static void *Run(void *arg);
};

#endif /* __SERVER_H__ */
