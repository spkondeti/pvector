#include "ne.h"
#include "router.h"


/* ----- GLOBAL VARIABLES ----- */
struct route_entry routingTable[MAX_ROUTERS];
int NumRoutes;
int deadNbr[MAX_ROUTERS];


////////////////////////////////////////////////////////////////
void InitRoutingTbl (struct pkt_INIT_RESPONSE *InitResponse, int myID){
  /* ----- YOUR CODE HERE ----- */
  int nbr_counter = 0;
  //Add all neighbors
  // bzero(InitResponse->path,sizeof(InitResponse->path[0]));
  while(nbr_counter < InitResponse->no_nbr){
    routingTable[nbr_counter+1].dest_id = InitResponse->nbrcost[nbr_counter].nbr;
    routingTable[nbr_counter+1].next_hop = InitResponse->nbrcost[nbr_counter].nbr;
    routingTable[nbr_counter+1].cost = InitResponse->nbrcost[nbr_counter].cost;    
    routingTable[nbr_counter+1].path_len = 2;
    routingTable[nbr_counter+1].path[0] = myID;
    routingTable[nbr_counter+1].path[1] = InitResponse->nbrcost[nbr_counter].nbr;
    nbr_counter++;
  }
  int i;
  for (i=0;i<MAX_ROUTERS;i++) {
    deadNbr[i] = 0;
  }
  
  //Add router's own path
  routingTable[0].dest_id = myID;
  routingTable[0].next_hop = myID;
  routingTable[0].cost = 0;    
  routingTable[0].path_len = 1;
  routingTable[0].path[0] = myID;
  //??? PATHVECTOR?? IFDEF??
  //??? MAX_ROUTERS?? 10 or num of neighbors
  NumRoutes = 1+InitResponse->no_nbr;
  return;
}


////////////////////////////////////////////////////////////////
int UpdateRoutes(struct pkt_RT_UPDATE *RecvdUpdatePacket, int costToNbr, int myID){
  /* ----- YOUR CODE HERE ----- */
  int router_counter = 0;
  int temp1 = 0, temp2=0; //var to go through all my router rows 
  int found = 0; //var to indicate if router is present in my table (Default Not Found)
  int counter = 0; //counter to append to the path
  int num_to_return = 0;
  int found_in_path = 0;
  //bzero(RecvdUpdatePacket->route.path,sizeof(RecvdUpdatePacket->route.path[0]));
  deadNbr[RecvdUpdatePacket->sender_id] = 0;
  while(router_counter < RecvdUpdatePacket->no_routes){
    temp1 = 0;
    while(temp1 < NumRoutes){
      if(routingTable[temp1].dest_id == RecvdUpdatePacket->route[router_counter].dest_id){
	found = 1;//Found
	break;
      }
      temp1++;
    }
    if (found == 0){ //If not found
      NumRoutes += 1;
      routingTable[temp1].dest_id = RecvdUpdatePacket->route[router_counter].dest_id;
      routingTable[temp1].next_hop = RecvdUpdatePacket->sender_id;
      routingTable[temp1].cost = RecvdUpdatePacket->route[router_counter].cost + costToNbr; //cost = cost to sender + cost from sender to dest
      routingTable[temp1].path_len = 1+routingTable[RecvdUpdatePacket->sender_id].path_len; //path_len = 1 + path_len from sender to dest
      counter = 0;
      routingTable[temp1].path[0] = myID;
      while(counter < RecvdUpdatePacket->route[router_counter].path_len){
	routingTable[temp1].path[counter+1] = RecvdUpdatePacket->route[router_counter].path[counter];
	counter++;
      }
      num_to_return = 1; //Returning 1 because we changed the routing table
    }
    else{   //If found
      temp2 = 0;
      //////////////Path vector rule starts////////
      
      while(temp2 < RecvdUpdatePacket->route[router_counter].path_len){
	if(RecvdUpdatePacket->route[router_counter].path[temp2] == myID){ //If sender has my ID in its path to destination
	  found_in_path = 1;
	}
	temp2++;
      }
      if(!found_in_path){
	if (routingTable[temp1].cost > RecvdUpdatePacket->route[router_counter].cost + costToNbr){
	  int deadflag = 0;
	  int i;
	  for(i=0; i<RecvdUpdatePacket->route[router_counter].path_len; i++){
	    if(deadNbr[RecvdUpdatePacket->route[router_counter].path[i]]) {
	      deadflag = 1;
	      break;
	    }
	  }
	  
	  if ((deadNbr[routingTable[temp1].dest_id]&&(routingTable[temp1].dest_id != RecvdUpdatePacket->sender_id)) || deadflag) {
	    ;
	  }
	  else {
	    counter = 0;
	    routingTable[temp1].path[0] = myID;
	    while(counter <  RecvdUpdatePacket->route[router_counter].path_len){
	      routingTable[temp1].path[counter+1] = RecvdUpdatePacket->route[router_counter].path[counter];
	      counter++;
	    }
	    routingTable[temp1].cost = RecvdUpdatePacket->route[router_counter].cost + costToNbr;
	    routingTable[temp1].path_len = RecvdUpdatePacket->route[router_counter].path_len+1;
	    routingTable[temp1].next_hop = RecvdUpdatePacket->sender_id;
	    num_to_return = 1;
	  }
	  }
	////////PATH VECTOR RULE END//////////////
	//FORCED UPDATE START///
	else if(routingTable[temp1].next_hop == RecvdUpdatePacket->sender_id){
	  int temp = RecvdUpdatePacket->route[router_counter].cost + costToNbr;
	  if (temp > INFINITY){
	    temp = INFINITY;
	  }
	  counter = 0;
	  if(temp > routingTable[temp1].cost){
	    routingTable[temp1].cost = temp;
	    routingTable[temp1].path[0] = myID;
	    routingTable[temp1].path_len = RecvdUpdatePacket->route[router_counter].path_len+1;
	    while(counter <  RecvdUpdatePacket->route[router_counter].path_len){
	      routingTable[temp1].path[counter+1] = RecvdUpdatePacket->route[router_counter].path[counter];

	      counter++;
	    }
	    num_to_return = 1;
	  }
	  else{	    
	    routingTable[temp1].path[0] = myID;
	    routingTable[temp1].path_len = RecvdUpdatePacket->route[router_counter].path_len+1;
	    while(counter <  RecvdUpdatePacket->route[router_counter].path_len){
	      if(routingTable[temp1].path[counter+1] != RecvdUpdatePacket->route[router_counter].path[counter]){
		routingTable[temp1].path[counter+1] = RecvdUpdatePacket->route[router_counter].path[counter];
		num_to_return = 1;
	      }
	      counter++;
	    }	 
	  }
	 
	}
      }
      found_in_path = 0;
      router_counter++;
      found = 0;
    }
  }
  return num_to_return;

  //Is Max_Routers 10 or no_routes
}


////////////////////////////////////////////////////////////////
void ConvertTabletoPkt(struct pkt_RT_UPDATE *UpdatePacketToSend, int myID){
  /* ----- YOUR CODE HERE ----- */
  UpdatePacketToSend -> sender_id = myID;
  UpdatePacketToSend -> no_routes = NumRoutes;
  int counter = 0;
  for(counter = 0; counter < NumRoutes; counter++){
    UpdatePacketToSend -> route[counter] = routingTable[counter];
  }
  return;
}


////////////////////////////////////////////////////////////////
//It is highly recommended that you do not change this function!
void PrintRoutes (FILE* Logfile, int myID){
  /* ----- PRINT ALL ROUTES TO LOG FILE ----- */
  int i;
  int j;
  for(i = 0; i < NumRoutes; i++){
    fprintf(Logfile, "<R%d -> R%d> Path: R%d", myID, routingTable[i].dest_id, myID);

    /* ----- PRINT PATH VECTOR ----- */
    for(j = 1; j < routingTable[i].path_len; j++){
      fprintf(Logfile, " -> R%d", routingTable[i].path[j]);	
    }
    fprintf(Logfile, ", Cost: %d\n", routingTable[i].cost);
  }
  fprintf(Logfile, "\n");
  fflush(Logfile);
}


////////////////////////////////////////////////////////////////
void UninstallRoutesOnNbrDeath(int DeadNbr){
  /* ----- YOUR CODE HERE ----- */
  int counter = 0;
  deadNbr[DeadNbr] = 1;
  while(counter < NumRoutes){
    if(routingTable[counter].next_hop == DeadNbr){
      routingTable[counter].cost = INFINITY;
      //routingTable[counter].next_hop = -1;
      //routingTable[counter].path[0] = ;
    }
    counter++;
  }
  return;
}
