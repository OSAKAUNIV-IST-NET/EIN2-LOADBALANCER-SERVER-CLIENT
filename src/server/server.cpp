#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>
#include <sys/file.h>
#include <netinet/ip.h> 

#include "common.h"
#include "server.h"

bool Server::is_active = true;
int Server::max_life_ = 0;
FILE *Server::life_file_ = NULL;

Server::Server(const char *ip_addr, int port, int port_ack, int max_life, int dec_life, int sleep_time, FILE *file)
  : n_rcv_pkts(0), life(max_life)
{
  strcpy(this->ip_addr, ip_addr);
  this->port = port;
  this->port_ack = port_ack;
  this->max_life = max_life;
  this->decrement_life = dec_life;
  this->sleep_time = sleep_time;
  this->life_file = file;
  this->max_life_ = max_life; // for signal handling
  this->life_file_ = file; // for signal handling

#ifdef DEBUG
  cerr << "MAX_LIFE[" << ip_addr << "]: " << max_life << endl;
  cerr << "DEC_LIFE[" << ip_addr << "]: " << decrement_life << endl;
  cerr << "SLEEP_TM[" << ip_addr << "]: " << sleep_time << endl;
#endif
  
  /* check my own address */
  my_ipv4_address = ipv4_char_to_uint32(this->ip_addr);

#ifdef DEBUG
  /* print address */
  unsigned char octet_[4]  = {0,0,0,0};
  for (int i=0; i<4; i++) {
    octet_[i] = ( my_ipv4_address >> (i*8) ) & 0xFF;
  }
  fprintf(stderr, "my_ipv4_address:  %d.%d.%d.%d\n",octet_[0],octet_[1],octet_[2],octet_[3]);
#endif

  /* Set amount of decrement of life point */
//   srand((unsigned)ip_addr);
//   decrement_life = (rand() % MAX_DEC_LIFE + MIN_DEC_LIFE);
// #ifdef DEBUG
//   cerr << "DEC_LIFE[" << ip_addr << "]: " << decrement_life << endl;
// #endif

  /* Output life */
  output_life(life, life_file);

}

Server::Server(server_t *srv)
  : n_rcv_pkts(0), life(0)
{
  config_t config = Config::GetInstance()->getConfig();

  /* Initialize */
  strcpy(ip_addr, srv->ip_addr.c_str());
  this->port = srv->port;
}

Server::~Server() {}

// void Server::Score(vector<Server *> srv_list)
// {
//   double score = 0;

//   /* Calculate score */
//   for(int i=0; i<srv_list.size(); i++){
//     Server *s = srv_list[i];
//     score += s->getNumRecvPkts();
//   }


//   /* Show score */
//   fprintf(stderr, "Score: %f\n", score);
// }

void *Server::Run(void *arg)
{
  Server *srv = reinterpret_cast<Server *>(arg);

  srv->run();
}


void Server::recv_udp_packets()
{
  int sock, s_ipc;
  struct sockaddr_in addr;
  struct sockaddr_un addr_ipc;
  fd_set active_fd_set, read_fd_set;


  setup_udp_server(&sock, &addr);
  setup_ipc_server(&s_ipc, &addr_ipc);

  FD_ZERO(&active_fd_set);
  FD_SET(sock, &active_fd_set);
  FD_SET(s_ipc, &active_fd_set);


  while(1){
    read_fd_set = active_fd_set;
    if (select(FD_SETSIZE, &read_fd_set, NULL, NULL, NULL) < 0){
      perror("Server: select");
      exit (-1);
    }
    for(int i=0; i < FD_SETSIZE; ++i){
      if(FD_ISSET(i, &read_fd_set)){
	if(i == sock) {
	  // for UDP
	  recv_packets(sock, addr);
	} else if (i == s_ipc) {
	  // for IPC
	  int n_sock = accept_ipc_server(s_ipc, addr_ipc);
	  FD_SET (n_sock, &active_fd_set);
	} else {
	  ofg_state stat = proc_ipc(i);
	  if(stat == FIN) {
	    /* close socks */
	    close(sock);
	    close(s_ipc);
	    close(i);

	    FD_CLR(sock, &active_fd_set);
	    FD_CLR(sock, &active_fd_set);
	    FD_CLR(i, &active_fd_set);

	    const char *ipc_addr = Server::IPC_Addr(this->ip_addr, this->port);
	    unlink(ipc_addr);

	    return;
	  }
	}
      }
    }
  }
}

void Server::setup_udp_server(int *sock, struct sockaddr_in *addr)
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
  addr->sin_port = htons(this->port);
  addr->sin_addr.s_addr = INADDR_ANY;

  // bind
  if( bind(*sock, (struct sockaddr *)addr, sizeof(*addr)) < 0) {
    perror("Server: bind");
    exit(-3);
  }
}

void Server::setup_ipc_server(int *sock, struct sockaddr_un *addr)
{
  // create socket
  if((*sock = socket(AF_UNIX, SOCK_STREAM, 0)) < 0 ) {
    perror("Server: socket");
    exit(-1);
  }
  const char *ipc_addr = Server::IPC_Addr(this->ip_addr, this->port);


  addr->sun_family = AF_UNIX;
  strcpy(addr->sun_path, ipc_addr);
  
  unlink(ipc_addr);

  // bind
  if (bind(*sock, (struct sockaddr *)addr, SUN_LEN(addr)) < 0) {
    perror("Server: bind");
    exit(-1);
  }

  // listen
  if (listen(*sock, 5) < 0){
    perror("Server: listen");
    exit(-1);
  }
}

int Server::accept_ipc_server(int sock, struct sockaddr_un addr)
{
  int n_sock;
  unsigned int size;

  size = sizeof(addr);

  if((n_sock = accept(sock, (sockaddr *)&addr, &size)) < 0) {
    perror("Server: accept");
    exit (-1);
  }

  return n_sock;
}

void Server::recv_packets(int sock, struct sockaddr_in addr)
{
  char recv_buf[RECV_BUFF];
  struct sockaddr_in clt;
  unsigned int sin_size = sizeof(struct sockaddr_in);  
  int msg_size;
  memset(recv_buf, 0 , sizeof(recv_buf));
  msg_size = recvfrom(sock, recv_buf, sizeof(recv_buf), 0, (struct sockaddr *)&clt, &sin_size);

  struct iphdr *ip; 
  ip = (struct iphdr *)recv_buf;

  /* Increment number of received packets */
  n_rcv_pkts++;

  /* 
   * Decrement life point and output it to file.
   * Then, add or decrement score.
   * Finally, begin to sleep, if life point become zero.
   */
  if (is_active) {
    /* First visit after sleep, if is_active is true and life is zero */
    /* Then, life will be set at MAX_LIFE */
    if (life == 0) {
#ifdef VERBOSE
      cerr << "[" << ip_addr << "] Came back" << endl;
#endif
      life = max_life;
    }

    /*
     * send ACK back for all received packet
     * to same port as incomming packet
     */
    struct sockaddr_in addr_ack;
    addr_ack.sin_family = AF_INET;
    addr_ack.sin_port = htons(this->port_ack);
    addr_ack.sin_addr = clt.sin_addr;
    sendto(sock, recv_buf, msg_size, 0, (struct sockaddr *)&addr_ack, sizeof(addr_ack));

    /* Decrement life point */
    /* Life point will be set at zero, if it went negative */
    life -= decrement_life;
    if (life < 0) {
      life = 0;
    }
#ifdef VERBOSE
    cerr << "[" << ip_addr << "] Receive packet from " << inet_ntoa(clt.sin_addr) << " (life: " << life << ")" << endl;
    cerr << "[" << ip_addr << "] `-> " << recv_buf << endl;
#endif
    /* Add score */
    score = score + 1.0;
  } else {
#ifdef VERBOSE
    cerr << "[" << ip_addr << "] INACTIVE: Receive packet from " << inet_ntoa(clt.sin_addr) << endl;
    cerr << "[" << ip_addr << "] `-> " << recv_buf << endl;
#endif
    /* Decrement score */
    score = score - 1.0;
    return;
  }

  /* Output life */
  //output_life(life, life_file);
  output_life(life);

  /* Sleep, if life point is zero */
  if (life == 0) {
    is_active = false;
    /* used for RANDOM period sleep */
    // int v = (rand() % MAX_SLEEP_TIME + MIN_SLEEP_TIME);
#ifdef VERBOSE
    cerr << "[" << ip_addr << "] Sleep:" << sleep_time << "[s]" << endl;
#endif
    signal(SIGALRM, Server::sigalrm_handler); /* Install the handler */
    alarm(sleep_time);
  }
}

ofg_state Server::proc_ipc(int sock)
{
  char recv_buf[RECV_BUFF];
  int nbytes;

  memset(recv_buf, 0, sizeof(recv_buf));
  
  nbytes = read(sock, recv_buf, sizeof(recv_buf));

  if(nbytes < 0) {
    perror("Server: read");
    exit(EXIT_FAILURE);
  } else if(nbytes == 0) {
    // return FIN;
  } else {
    if(strcmp(recv_buf, "FIN")==0){
      // send_msg_info(sock);
      return FIN;
    }
  }

  return RUN;
}

void Server::send_msg_info(int sock)
{
  char msg[TMP_BUF_SIZE];

  sprintf(msg, "N_RCV_PKTS:%u,LIFE:%d\n", this->n_rcv_pkts, this->life);

  write(sock, msg, strlen(msg)+1);
}

void Server::run()
{
  recv_udp_packets();
}

void Server::show_score()
{
  /* Show score */
  //  fprintf(stdout, "Score: %f\n", this->n_rcv_pkts/1.0);
  fprintf(stdout, "Score: %f\n", this->score/1.0);
  // fprintf(stderr, "Score(%s:%d): %f\n", this->ip_addr, this->port, this->n_rcv_pkts/1.0);
}

void Server::sigalrm_handler(int sig)
{
#ifdef DEBUG
  cerr << "[             ] signal" << endl;
#endif
  Server::is_active = true;

  /* Initialize life point file by writing MAX_LIFE on the file */
  if (flock(fileno(life_file_), LOCK_EX) == 0) {
    ftruncate(fileno(life_file_), 0);
    fprintf(life_file_, "%d\n", max_life_);
    fflush(life_file_);
    flock(fileno(life_file_), LOCK_UN);
  } else {
    cerr << "ERROR: flock" << endl;
  }

}

bool Server::output_life(int life, FILE *file_life)
{
  if (flock(fileno(life_file), LOCK_EX) == 0) {
    ftruncate(fileno(life_file), 0);
    fprintf(life_file, "%d\n", life);
    fflush(life_file);
    flock(fileno(life_file), LOCK_UN);
  } else {
    cerr << "ERROR: flock" << endl;
    return false;
  }
  return true;
}
bool Server::output_life(int life)
{
  FILE *life_file;
  if((life_file = fopen(life_filename.c_str(), "w")) == NULL){
    cerr << "ERROE: fopen" << endl;
    fclose(life_file);
    exit(1);
  }
  if (flock(fileno(life_file), LOCK_EX) == 0) {
    ftruncate(fileno(life_file), 0);
    fprintf(life_file, "%d\n", life);
    fflush(life_file);
    flock(fileno(life_file), LOCK_UN);
  } else {
    cerr << "ERROR: flock" << endl;
    return false;
  }
  fclose(life_file);
  return true;
}

uint32_t Server::ipv4_char_to_uint32(char *ip_addr)
{
  unsigned int octet[4] = {0, 0, 0, 0};
  int octet_index = 0;
  uint32_t ipv4_address;
  for (int i = 0; ip_addr[i] != 0; i++) {
    if (ip_addr[i] != '.') {
      octet[octet_index] = octet[octet_index] * 10 + (ip_addr[i] - '0');
    } else {
      octet_index++;
    }
  }
  ipv4_address = 0;
  for (int i=3; i>=0; i--) {
    ipv4_address = (ipv4_address << 8) | octet[i];
   }
  return ipv4_address;
}
