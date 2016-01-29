#include "betting_server.h"

int main(void)
{
   int error;
   pthread_t tid[2]; //thread ids

   if (pthread_mutex_init(&mtx, NULL) != 0)
   {
      return -1;
   }

   error = pthread_create(&tid[0], NULL, &begasep_tcp_server, NULL);
   if(error != 0)
   {
      TRACE_LOG("Cannot create begasep_tcp_server thread\n");
   }
   else
   {
      TRACE_LOG("begasep_tcp_server thread created successfully\n");
   }

   error = pthread_create(&tid[1], NULL, &begasep_bet_server, NULL);
   if(error != 0)
   {
      TRACE_LOG("Cannot create begasep_bet_server thread\n");
   }
   else
   {
      TRACE_LOG("begasep_bet_server thread created successfully\n");
   }

   if(pthread_join(tid[1], NULL)) //wait second thread to finish
      TRACE_LOG("Error joining thread\n");

   pthread_mutex_destroy(&mtx); // never called but put here for consistency
   return 0;
}
