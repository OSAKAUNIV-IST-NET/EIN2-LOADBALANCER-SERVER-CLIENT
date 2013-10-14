#ifndef __CLIENT_H__
#define __CLIENT_H__

/**
 * Header file for Client 
 */

#include <vector>
#include <sys/socket.h>
#include <sys/un.h>

#include "common.h"

//#define DEBUG
//#define VERBOSE

using namespace std;

struct results_t{
  unsigned int n_rcv_pkts;
  double life;
};

/* struct sndpkts_arg_t { */
/*   server_t *srv; */
/*   int n_pkts; */
/* }; */

class Client
{
 private:
  FRIEND_TEST(ClientTest, initialize);

  int port_ack;
  int num_pkts;
  char ip_addr[20];
  int min_intval;
  int max_intval;
  vector<server_t*> srv_array;

  results_t results;

  int ack_timeout;
  int duration;

  void initialize(client_t *client, server_t * server, int num_pkts, int port_ack, int duration);

 protected:

  void setup_udp_client(int *sock, struct sockaddr_in *addr,
			const char *ip_addr, int port);
  void setup_ipc_client(int *sock, struct sockaddr_un *addr,
			const char *ip_addr, int port);
  void setup_ack_server(int *sock, struct sockaddr_in *addr, int port);
  void send_udp_packets(const char *ip_addr, int port, int n_pkts, int port_ack);
  void send_packets(int sock, struct sockaddr_in addr, int n_pkts, int sock_ack);
  void send_msg_finish(int sock);
  void recv_msg_info(int sock);


 public:
  Client(client_t *client, server_t *server);
  ~Client();

  /* static void Show(vector<Client *> clients); */

  static void *Run(void *arg);
  void run();

};

#endif /* __CLIENT_H__ */
