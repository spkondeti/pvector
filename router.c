#include <stdio.h>
#include "ne.h"
#include "router.h"
#include <stdlib.h>
#include <unistd.h> 
#include <string.h>
#include <netinet/in.h>
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <time.h>
#include <pthread.h>
struct nbr_cost_time{
  unsigned int nbr;
  unsigned int cost;
  time_t lupd;
  unsigned int death;
};
pthread_mutex_t lock;
void * sendFn(void * arg);
void * recvFn(void *arg);
struct nbr_cost_time mynbr[MAX_ROUTERS - 1];
time_t conv_timer;
unsigned int myID;
int sockfd;
struct sockaddr_in servaddr;
socklen_t temp_var;
int num_nbrs = 0;
FILE *fp;
time_t init_timer;
int conv_flag = 0;

int main(int argc, char ** argv){
  if (argc != 5){
    return 0;
  } 
  unsigned int router_id = atoi(argv[1]);
  myID = router_id;
  char* ne_host = argv[2];
  int ne_port = atoi(argv[3]);
  int router_port = atoi(argv[4]);
  struct pkt_INIT_REQUEST init_req;
  struct sockaddr_in sa,ra;
  
  init_req.router_id = htonl(router_id);
  if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) { 
    exit(1); 
  }
  memset(&sa, 0, sizeof(struct sockaddr_in));
  ra.sin_family = AF_INET;
  ra.sin_addr.s_addr = INADDR_ANY;
  ra.sin_port = htons(router_port);

  if (bind(sockfd, (struct sockaddr *)&ra, sizeof(struct sockaddr_in)) == -1){

    //printf("Bind to Port Number %d ,IP address %s failed\n",RECEIVER_PORT_NUM,RECEIVER_IP_ADDR);
      close(sockfd);
      exit(1);
  }
  struct hostent *hp; 
  memset(&servaddr, 0, sizeof(servaddr)); 
  if ((hp = gethostbyname(ne_host)) == NULL){
    exit(0);
  }

  // Filling server information 
  bzero((char *) &servaddr, sizeof(servaddr)); 
  servaddr.sin_family = AF_INET; 
  bcopy((char *)hp->h_addr, (char *)&servaddr.sin_addr.s_addr, hp->h_length); 
  servaddr.sin_port = htons(ne_port); 
  temp_var = sizeof(servaddr);
  sendto(sockfd,&init_req,sizeof(struct pkt_INIT_REQUEST),0,(const struct sockaddr *)&servaddr,temp_var);
  struct pkt_INIT_RESPONSE init_resp;
  recvfrom(sockfd,&init_resp,sizeof(struct pkt_INIT_RESPONSE),0,(struct sockaddr*)&servaddr,&temp_var);

  init_timer = time(NULL);
  conv_timer = init_timer;
  ntoh_pkt_INIT_RESPONSE(&init_resp);
  InitRoutingTbl(&init_resp,router_id);
  char buffer[100];
  sprintf(buffer,"router%d.log",myID);
  fp = fopen(buffer,"w");
  PrintRoutes(fp,myID);
  
  int counter = 0;
  while(counter < init_resp.no_nbr){
    mynbr[counter].nbr = init_resp.nbrcost[counter].nbr;
    mynbr[counter].cost = init_resp.nbrcost[counter].cost;    
    mynbr[counter].lupd = time(NULL);
    mynbr[counter].death = 0;
    num_nbrs++;
    counter++;
  }
  
  pthread_t poll_thread;
  pthread_t timer_thread;
  pthread_mutex_init(&lock, NULL);
  if(pthread_create(&poll_thread, NULL, recvFn, NULL)){
    exit(0);
  }
  if(pthread_create(&timer_thread, NULL, sendFn, NULL)){
    exit(0);
  }
  pthread_join(poll_thread, NULL);
  pthread_join(timer_thread, NULL);
  return 0;
}

void * recvFn(void * arg){
  struct pkt_RT_UPDATE update_packet;
  int counter1;
  int is_updated = 0;
  while(1){
    recvfrom(sockfd,&update_packet,sizeof(struct pkt_RT_UPDATE),0,(struct sockaddr*)&servaddr,&temp_var);
    ntoh_pkt_RT_UPDATE(&update_packet);
    pthread_mutex_lock(&lock);
    counter1 = 0;
    while(counter1 < update_packet.no_routes){
      if(mynbr[counter1].nbr == update_packet.sender_id){
	break;
      }
      counter1++;
    }
    is_updated = UpdateRoutes(&update_packet,mynbr[counter1].cost,myID);
    mynbr[counter1].lupd = time(NULL);
    mynbr[counter1].death = 0;
    if(is_updated){  //Change last updated for this nbr
      conv_flag = 0;
      conv_timer = time(NULL);
      PrintRoutes(fp,myID);
    }
    pthread_mutex_unlock(&lock);
  }
  fclose(fp);
  return NULL;
}


void * sendFn(void * arg){
  struct pkt_RT_UPDATE send_packet;
  time_t start_time = time(NULL);
  int i,j;
  char buffer[100];
  int k;
  while(1){
    pthread_mutex_lock(&lock);
    for(j = 0; j < num_nbrs; j++){
      if(!(mynbr[j].death) && (time(NULL) - mynbr[j].lupd >= FAILURE_DETECTION)){
	UninstallRoutesOnNbrDeath(mynbr[j].nbr);
	mynbr[j].death = 1;
	conv_flag = 0;
	conv_timer = time(NULL);
	PrintRoutes(fp,myID);
      }
    }
    if(time(NULL) - start_time >= UPDATE_INTERVAL){
      bzero(&send_packet,sizeof(send_packet));
      ConvertTabletoPkt(&send_packet,myID);
      hton_pkt_RT_UPDATE(&send_packet);
      for(i = 0; i < num_nbrs; i++){
	send_packet.dest_id = htonl(mynbr[i].nbr);
        sendto(sockfd,&send_packet,sizeof(struct pkt_RT_UPDATE),0,(const struct sockaddr *)&servaddr,temp_var);
      }
      start_time = time(NULL);
    }
    
    k = (int)(time(NULL) - conv_timer);
    if((k  >= CONVERGE_TIMEOUT) && !conv_flag){
      conv_flag = 1;
      sprintf(buffer,"%d:Converged\n",(int)(time(NULL) - init_timer));
      fputs(buffer,fp);
      fflush(fp);
    }    
    pthread_mutex_unlock(&lock);
  }
  return NULL;
}
