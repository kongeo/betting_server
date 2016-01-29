#include "betting_server.h"

// GLOBALS
client client_rec[BEGASEP_NUM_CLIENTS]; //client records array
int total_clients = 0; //number of active clients
fd_set master_fds; //socket fd master list

// FUNCTION DECLARATION
static int lookup_index(int socket_fd);
static int listen4clients(struct sockaddr_in* sockaddress, int port);
static int close_connection(int id);
static int respond_accept(int fd, int id);
static int respond_result(int id, long int win_num);

// FUNCTION DEFINITION
static int lookup_index(int socket_fd)
{
   int i;
   pthread_mutex_lock(&mtx);
   for(i=0; i < BEGASEP_NUM_CLIENTS; i++)
   {
      if(client_rec[i].sockfd == socket_fd)
      {
         pthread_mutex_unlock(&mtx);
         return i;
      }
   }
   pthread_mutex_unlock(&mtx);
   return -1;
}

static int listen4clients(struct sockaddr_in* sockaddress, int port)
{
   int sock = socket(AF_INET, SOCK_STREAM, 0);

   if(sock < 0)
      return -1;

   sockaddress->sin_addr.s_addr = htonl(INADDR_ANY);
   sockaddress->sin_family = AF_INET;
   sockaddress->sin_port = htons(port);

   /* sin_zero needs to be cleared!!! */
   memset(sockaddress->sin_zero,0,8);

   if(bind(sock,
         (struct sockaddr *)sockaddress,
         sizeof(*sockaddress)) < 0)
      return -1;

   if(listen(sock, 0) < 0)
      return -1;

   return sock;
}

static int close_connection(int id)
{
   TRACE_LOG("begasep_tcp_server: close connection client_id:%d fd:%d\n", id, client_rec[id].sockfd);
   pthread_mutex_lock(&mtx);
   FD_CLR(client_rec[id].sockfd, &master_fds);
   close(client_rec[id].sockfd);
   client_rec[id].sockfd = -1;
   client_rec[id].state = BEGASEP_NONE;
   total_clients--;
   pthread_mutex_unlock(&mtx);
}

static int respond_accept(int fd, int id)
{
   void* accept = NULL;

   int length_accept = sizeof(begasep_common_header) + sizeof(begasep_accept);

   accept = malloc(length_accept);
   memset(accept, 0, sizeof(length_accept));

   ((begasep_common_header*)accept)->ver = 1;
   ((begasep_common_header*)accept)->len = length_accept;
   ((begasep_common_header*)accept)->type = BEGASEP_ACCEPT;
   ((begasep_common_header*)accept)->client_id = (id);
   accept += sizeof(begasep_common_header);
   ((begasep_accept*)accept)->range_lwr_end = (BEGASEP_NUM_MIN);
   ((begasep_accept*)accept)->range_upr_end = (BEGASEP_NUM_MAX);
   accept -= sizeof(begasep_common_header);

   send(fd, accept, length_accept, 0);

   pthread_mutex_lock(&mtx);
   client_rec[id].state = BEGASEP_ACCEPT;
   pthread_mutex_unlock(&mtx);
}

void* begasep_tcp_server()
{
   int i;
   int server_fd = 0; //server socket keeper
   int bytes_read = 0;
   char recv_client_buf[BUF_LEN]; // buffer for reading sockets
   begasep_common_header* header;
   begasep_bet* bet;
   struct sockaddr_in server_sockaddr;
   struct sockaddr_storage client_addr;
   int client_addr_size = sizeof(client_addr);
   int client_id;
   fd_set read_fds; //temporary file desc list for select
   int fd_max;

   memset(client_rec,-1, BEGASEP_NUM_CLIENTS * sizeof(client));

   /* initialize fd groups */
   pthread_mutex_lock(&mtx);
   FD_ZERO(&master_fds);
   pthread_mutex_unlock(&mtx);
   FD_ZERO(&read_fds);

   /* open a listening socket*/
   server_fd = listen4clients(&server_sockaddr, TCP_PORT);

   /* add server_fd to the fd master set */
   pthread_mutex_lock(&mtx);
   FD_SET(server_fd, &master_fds);
   pthread_mutex_unlock(&mtx);

   fd_max = server_fd;

   /* loop through and detect activity in sockets */
   while(1)
   {
      read_fds = master_fds;
      if(select(fd_max+1, &read_fds, NULL, NULL, NULL) < 0)
      {
         perror("select");
         TRACE_LOG("begasep_tcp_server: smth bad in select close all active connections\n");
         for(i=0; i <= BEGASEP_NUM_CLIENTS; i++)
         {
            close_connection(i);
         }
      }
      for(i=0; i <= fd_max; i++)
      {
         if(FD_ISSET(i,&read_fds))
         {
            if (i == server_fd) //data to read on the server socket
            {
               client_id = lookup_index(-1); //find first slot available
               if(total_clients < BEGASEP_NUM_CLIENTS)
               {

                  /* accept connection from client */
                  pthread_mutex_lock(&mtx);
                  client_rec[client_id].sockfd = accept(server_fd,
                        (struct sockaddr *)&client_addr,
                        &client_addr_size);
                  if(client_rec[client_id].sockfd < 0)
                  {
                     perror("accept");
                  }
                  else
                  {
                     TRACE_LOG("begasep_tcp_server: client accepted on %d\n", client_rec[client_id].sockfd);
                     FD_SET(client_rec[client_id].sockfd, &master_fds);
                     total_clients++;
                  }
                  pthread_mutex_unlock(&mtx);

                  if(client_rec[client_id].sockfd > fd_max)
                        fd_max = client_rec[client_id].sockfd;
               }
               else
                  TRACE_LOG("begasep_tcp_server: cannot accept more connections");
            }
            else
            {
               memset(recv_client_buf,0,BUF_LEN); //clear receive buffer

               bytes_read = recv(i, recv_client_buf, BUF_LEN, 0);
               if(bytes_read > 0)
               {
                  header = (begasep_common_header*)recv_client_buf;
                  switch(header->type)
                  {
                     case BEGASEP_OPEN:
                        TRACE_LOG("begasep_tcp_server: open received by fd:%d client_id:%d\n", i, client_id);
                        respond_accept(i, client_id);
                        break;
                     case BEGASEP_BET:
                        bet = (void *)recv_client_buf + sizeof(begasep_common_header);
                        if(client_rec[client_id].state == BEGASEP_ACCEPT &&
                              ((begasep_bet*)bet)->bet_num <= BEGASEP_NUM_MAX &&
                              ((begasep_bet*)bet)->bet_num >= BEGASEP_NUM_MIN)
                        {
                           pthread_mutex_lock(&mtx);
                           client_rec[client_id].betting_num = ((begasep_bet*)bet)->bet_num;
                           client_rec[client_id].state = BEGASEP_BET;
                           pthread_mutex_unlock(&mtx);
                           TRACE_LOG("begasep_tcp_server: bet received by fd:%d client_id:%d bet_num:%lx\n", i, client_id, client_rec[client_id].betting_num);
                        }
                        else //protocol violation
                        {
                           close_connection(client_id);
                        }
                        break;
                     default:
                        close_connection(client_id);
                  }
               }
               else //some error occured or connection is closed by client
               {
                  TRACE_LOG("begasep_tcp_server: client might have closed the connection unexpectidly\n");
                  close_connection(client_id);
               }
            }
         }
      }
   }
   return NULL;
}

static int respond_result(int id, long int win_num)
{
   void* result = NULL;

   int length_result = sizeof(begasep_common_header) + sizeof(begasep_result);

   result = malloc(length_result);
   memset(result, 0, sizeof(length_result));

   ((begasep_common_header*)result)->ver = 1;
   ((begasep_common_header*)result)->len = length_result;
   ((begasep_common_header*)result)->type = BEGASEP_RESULT;
   ((begasep_common_header*)result)->client_id = id;
   result += sizeof(begasep_common_header);
   if(client_rec[id].betting_num == win_num)
      ((begasep_result*)result)->status = 1;
   else
      ((begasep_result*)result)->status = 0;
   ((begasep_result*)result)->win_num = win_num;
   result -= sizeof(begasep_common_header);

   send(client_rec[id].sockfd, result, length_result, 0);

   pthread_mutex_lock(&mtx);
   client_rec[id].state = BEGASEP_RESULT;
   pthread_mutex_unlock(&mtx);
}

void* begasep_bet_server()
{
   int i;
   long int winning_bet;
   while(1)
   {
      sleep(15);
      if(total_clients != 0)
      {
         srand(time(NULL));
         winning_bet = random()%(BEGASEP_NUM_MAX - BEGASEP_NUM_MIN) + BEGASEP_NUM_MIN;

         TRACE_LOG("begasep_bet_server: the winning number is: %lx\n", winning_bet);

         for(i = 0; i < BEGASEP_NUM_CLIENTS; i++)
         {
            if(client_rec[i].state == BEGASEP_BET)
            {
               respond_result(i, winning_bet);
               close_connection(i);
            }
         }
      }
   }
   return NULL;
}
