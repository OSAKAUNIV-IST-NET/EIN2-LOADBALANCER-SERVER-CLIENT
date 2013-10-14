#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <netinet/ip.h> 

#include "common.h"
#include "server.h"
#include "client.h"


/*
 * Constructor
 */
Client::Client(client_t *client, server_t *server)
  : min_intval(5), max_intval(50)
{
  config_t config = Config::GetInstance()->getConfig();

  initialize(client, server, config.num_pkts, server->port, config.duration);
}

void Client::initialize(client_t *client, server_t *server, int num_pkts, int port_ack, int duration)
{
  config_t config = Config::GetInstance()->getConfig();

  /* initialize */
  strcpy(this->ip_addr, client->ip_addr.c_str());
  this->num_pkts = num_pkts;
  this->srv_array.clear();
  this->srv_array.push_back(server);
  this->port_ack = port_ack;
  this->ack_timeout = client->ack_timeout;
  this->duration = duration;
  // results.life = 100;
  // results.n_rcv_pkts = 0;

#ifdef DEBUG
  cerr << "NUM_PKTS[" << ip_addr << "]: " << this->num_pkts << endl;
  cerr << "PORT_ACK[" << ip_addr << "]: " << this->port_ack << endl;
  cerr << "ACK_TOUT[" << ip_addr << "]: " << this->ack_timeout << endl;
  cerr << "DURATION[" << ip_addr << "]: " << this->duration << endl;
#endif

  /* Initialize seed */
  srand((unsigned long)time(NULL));
}

Client::~Client(){}

void *Client::Run(void *arg)
{
  Client *clt = reinterpret_cast<Client *>(arg);

  clt->run();
}

/**
 * Send UDP packets
 */
void Client::send_udp_packets(const char *ip_addr, int port, int n_pkts, int port_ack)
{
  int sock, s_ipc, sock_ack;
  struct sockaddr_in addr;
  struct sockaddr_un s_addr;
  struct sockaddr_in addr_ack;  

  // use same port as sending for receiving ack
  setup_udp_client(&sock, &addr, ip_addr, port);
  setup_ipc_client(&s_ipc, &s_addr, ip_addr, port);
  setup_ack_server(&sock_ack, &addr_ack, port_ack);

  send_packets(sock, addr, n_pkts, sock_ack);
  send_msg_finish(s_ipc);
  recv_msg_info(s_ipc);

  close(sock);
  close(s_ipc);
  close(sock_ack);
}

void Client::setup_udp_client(int *sock, struct sockaddr_in *addr, const char *ip_addr, int port)
{
  *sock = socket(AF_INET, SOCK_DGRAM, 0);

  addr->sin_family = AF_INET;
  addr->sin_port = htons(port);
  addr->sin_addr.s_addr = inet_addr(ip_addr);
}

void Client::setup_ipc_client(int *sock, struct sockaddr_un *addr, const char *ip_addr, int port)
{
  // create IPC socket
  if((*sock = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
    perror("Client: socket");
    exit (EXIT_FAILURE);
  }

  const char *ipc_addr = Server::IPC_Addr(ip_addr, port);

  addr->sun_family = AF_UNIX;
  strcpy(addr->sun_path, ipc_addr);

  // connect
  if(connect(*sock, (sockaddr *)addr, SUN_LEN(addr)) < 0){
    perror("Client: connect");
    exit (EXIT_FAILURE);
  }
}

void Client::setup_ack_server(int *sock, struct sockaddr_in *addr, int port_ack)
{

  /* Open socket by which all packet will be received */
  // *sock = socket(AF_PACKET, SOCK_DGRAM, htons(ETH_P_ALL)); 
  // if (sock < 0) { 
  //   perror("socket"); 
  //   return; 
  // } 

  /* ORIGINAL */
  // create socket
  if((*sock = socket(AF_INET, SOCK_DGRAM, 0))<0){
    perror("Server: socket");
    exit(-1);
  }

  addr->sin_family = AF_INET;
  addr->sin_port = htons(port_ack);
  addr->sin_addr.s_addr = INADDR_ANY;

  // bind
  if( bind(*sock, (struct sockaddr *)addr, sizeof(*addr)) < 0) {
    perror("Server: bind");
    exit(-3);
  }
}


/**
 * Send packets to the server
 */
void Client::send_packets(int sock, struct sockaddr_in addr, int n_pkts, int sock_ack)
{
  const char *msg = "hogehoge";

  /* 
   * used for handling ACK timeout
   */
  int maxfd = sock_ack;
  fd_set fds, readfds;
  int n;
  struct timeval tv;
  FD_ZERO(&readfds);
  FD_SET(sock_ack, &readfds);

  /*
   * used for handling time-up
   */
  time_t start_time;
  time(&start_time);
  time_t current_time;
  bool will_time_up = (duration > 0)?  true : false;

  /*
   * used for sending ACK
   */
  char recv_buf[RECV_BUFF];
  struct sockaddr_in clt;
  unsigned int sin_size = sizeof(struct sockaddr_in);  

  bool is_n_pkts_unlimited = (n_pkts > 0)?  false : true;
  // int v;
  for(int i=0; (is_n_pkts_unlimited) || (i < n_pkts); i++){
    /* calculate elapsed time, and check whether time is up or not
     * return if the time is has been over "duration" 
     */
    time(&current_time);    
    double elapsed_time = difftime(current_time, start_time);
    if (will_time_up && (elapsed_time > duration)) {
#ifdef VERBOSE
    cerr << "[" << ip_addr << "] time is up" << endl;
#endif     
      return;
    }

    sendto(sock, msg, strlen(msg), 0, (struct sockaddr *)&addr, sizeof(addr));
#ifdef VERBOSE
    cerr << "[" << ip_addr << "] Send packet to " << inet_ntoa(addr.sin_addr) << endl;
    cerr << "[" << ip_addr << "] `-> " << msg << endl;
#endif

    /* wait for reciving ACK */
    tv.tv_sec = ack_timeout;
    tv.tv_usec = 0;
    memcpy(&fds, &readfds, sizeof(fd_set));
    n = select(maxfd+1, &fds, NULL, NULL, &tv);
    if (n == 0) {
#ifdef VERBOSE
    cerr << "[" << ip_addr << "] ACK timeout occur" << endl;
#endif
      continue;
    }

    /* receive ACK */
    memset(recv_buf, 0 , sizeof(recv_buf));
    recvfrom(sock_ack, recv_buf, sizeof(recv_buf), 0, (struct sockaddr *)&clt, &sin_size);
    
#ifdef VERBOSE
    cerr << "[" << ip_addr << "] Receive ACK from " << inet_ntoa(clt.sin_addr) << endl;
    cerr << "[" << ip_addr << "] `-> " << recv_buf << endl;
#endif
    struct iphdr *ip; 
    ip = (struct iphdr *)recv_buf;

    // v = (rand() % max_intval + min_intval) * 1000;
    // usleep(v);
  }
  
}

void Client::send_msg_finish(int sock)
{
  const char *msg = "FIN";
  write(sock, msg, strlen(msg)+1);
}

void Client::recv_msg_info(int sock)
{
  char buf[TMP_BUF_SIZE];
  int nbytes;

  memset(buf, 0, sizeof(buf));
    
  nbytes = read(sock, buf, strlen(buf));

  if(nbytes < 0) {
    perror("Client: read");
    exit(EXIT_FAILURE);
  } else if(nbytes == 0){
    // return;
  } else{
    sscanf(buf, "N_RCV_PKTS:%u,LIFE:%lf\n", &results.n_rcv_pkts, &results.life);
  }
}

/**
 * Run client
 */
void Client::run()
{
  send_udp_packets(srv_array[0]->ip_addr.c_str(), srv_array[0]->port, this->num_pkts, port_ack);
  
  // // decide the number of packets sent to each server
  // vector<int> n_pkts_array;
  // int v;
  // for(int i=0; i<srv_array.size()-1; i++){
  //   v = this->num_pkts/srv_array.size();
  //   n_pkts_array.push_back(v);
  // }
  // n_pkts_array.push_back(this->num_pkts/srv_array.size() + this->num_pkts % srv_array.size());

  // vector<pthread_t *> thr_grp;
  // for(int i=0; i<srv_array.size(); i++){
  //   pthread_t *thr = new pthread_t;
  //   thr_grp.push_back(thr);
  //   sndpkts_arg_t sndpkts_arg;
  //   sndpkts_arg.srv = srv_array[i];
  //   sndpkts_arg.n_pkts = n_pkts_array[i];
  //   pthread_create(thr, NULL, Client::SendPackets, reinterpret_cast<void*>(sndpkts_arg));
  // }

  // pid_t pid;
  // // send packets to each server 
  // for(int i=0; i<srv_array.size(); i++){
  //   if((pid = fork()) == -1){
  //     perror("Client: fork");
  //     exit(EXIT_FAILURE);
  //   } else if(pid == 0){
  //     send_udp_packets(srv_array[i]->ip_addr.c_str(), srv_array[i]->port, n_pkts_array[i]);
  //     exit(EXIT_SUCCESS);
  //   }
  // }

  // // wait for all child processes.
  // wait_all("Client");
}

// void Client::Show(vector<Client *> clients)
// {
//   double score = 0;

//   /* Calculate score */
//   for(int i=0; i<clients.size(); i++){
//     Client *c = clients[i];
//     score += c->results.n_rcv_pkts;
//   }

//   /* Show score */
//   fprintf(stderr, "Score: %f\n", score);
// }
