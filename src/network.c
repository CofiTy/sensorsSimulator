#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <mqueue.h>
#include <signal.h>
#include <unistd.h>

#include "network.h"

int i, j;

/**
 * Full Duplex Socket for communication
 **/
int sock;
int acceptSock;

/**
 * Threads ID, reception and invoice for communication
 **/
pthread_t pthreadRec, pthreadSend;

/**
 * Message queue ID for invoice messages toward server
 **/
mqd_t mqSensorsSend;

/**
 * Thread Function: Receive Message
 **/
void * msgRec(){

  char buff[128];
  char data[32];
  int nb, total;
  char* receiving = (char *) buff;

  memset(buff, '\0', 128);
  total = 0;

  for(;;)
  {
    /* reception */
    nb = recv(sock, buff, 128, 0);
    FAIL(nb);

    total += nb;
    receiving += nb;

    i = 0;
    j = 0;
    while(i < (strlen(buff))){
      data[j++] = buff[i++];
      /* If enough data we can process */
      if(j == 28){
        printf("Trame toward activators : %s\n", data);
        j = 0;
        memset(data, '\0', 32);
      }
    }

    total = 0;
    receiving = (char *) buff;
    memset(buff, '\0', 128);
  }
}

/**
 * Thread Function: Sending Messages Toward server
 **/
void * msgSend(){

  char buff[8192];
  int nb, nbSent, total;
  char* sending = (char *) buff;

  for(;;)
  {
    /* Read Messages in MQueue */
    nb = mq_receive(mqSensorsSend, buff, 8192, 0);
    FAIL(nb);

    //printf("Sending toward Server: %s\n", buff);

    total = nb;
    nbSent = 0;
    while(nbSent < total)
    {
      /* Sending toward server */
      nb = send(sock, sending, nb, 0);
      FAIL(nb);
      nbSent += nb;
      sending += nb;
    }
    //puts("sent");
    sending = (char *) buff;
  }

}

/**
 * Initialisation of the Sensors Network
 **/
void initNetwork(){

  /* Create MessageQueue */
  mqSensorsSend = mq_open("/SensorsMsgSend", O_RDWR | O_CREAT, S_IRWXU, NULL);
  FAIL(mqSensorsSend);

  struct sockaddr_in saddr_client;
  struct sockaddr_in saddr;
  socklen_t size_addr = sizeof(struct sockaddr_in);
  memset(&saddr_client, 0, sizeof(struct sockaddr_in));

  saddr.sin_addr.s_addr = htonl(INADDR_ANY);
  saddr.sin_family = AF_INET;
  saddr.sin_port = htons(5042);

  /* Create The listening socket */
  acceptSock = socket(AF_INET, SOCK_STREAM, 0);
  FAIL(acceptSock);

  /* Bind Socket */
  FAIL(bind(acceptSock, (struct sockaddr *)&saddr, sizeof(saddr)));

  memset(&saddr, 0, sizeof(struct sockaddr_in));
    
  /* Listen */
  FAIL(listen(acceptSock, 10));
  puts("Listening");

  /* Accept GUI connexion */
  sock = accept(acceptSock, (struct sockaddr *)&saddr_client, &size_addr);
  FAIL(sock);
  puts("gotConnexion");

  /* Launch Thread Reception */
  pthread_create(&pthreadRec, NULL, msgRec, NULL);
  pthread_detach(pthreadRec);

  /* Launch Thread Sending */
  pthread_create(&pthreadSend, NULL, msgSend, NULL);
  pthread_detach(pthreadSend);

}

/**
 * Desactivation of Sensors Network and free related memory
 **/
void closeNetwork(){

  FAIL(pthread_cancel(pthreadRec));
  FAIL(pthread_cancel(pthreadSend));
  FAIL(close(sock));
  FAIL(close(acceptSock));
  FAIL(mq_close(mqSensorsSend));
}

/**
 * Send Message toward Sensors
 **/
int networkSendTrame(const char * msg_ptr, size_t msg_len){
  return mq_send( mqSensorsSend, msg_ptr, msg_len, 0);
}
