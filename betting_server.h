/** @file cqueue_api.h
 *
 * @author     Konstantinos Georgantas
 * @date       March 2015
 * @defgroup   BEGASEP Betting Game Server Protocol
 *
 * @addtogroup BEGASEP
 * @{
 * @brief Definition of BEGASEP
 */
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

/*!
 * @ingroup BEGASEP
 * @var     mtx
 * @brief   global mutex for thread sync
 */
pthread_mutex_t mtx;

/*!
 * @ingroup BEGASEP
 * @def     BEGASEP_NUM_CLIENTS
 * @brief   Define @a BEGASEP_NUM_CLIENTS to indicate number of accepted
 * clients
 */
#define BEGASEP_NUM_CLIENTS  40 //must allow from 1 to 64500
/*!
 * @ingroup BEGASEP
 * @def     BEGASEP_NUM_MIN
 * @brief   Define @a BEGASEP_NUM_MIN to indicate minimum betting number
 */
#define BEGASEP_NUM_MIN      0xe0ffffa0
/*!
 * @ingroup BEGASEP
 * @def     BEGASEP_NUM_MAX
 * @brief   Define @a BEGASEP_NUM_MIN to indicate maximum betting number
 */
#define BEGASEP_NUM_MAX      0xe0ffffaa
/*!
 * @ingroup BEGASEP
 * @def     TCP_PORT
 * @brief   Define @a TCP_PORT to indicate the port that the server will be
 * listening on
 */
#define TCP_PORT 2222
/*!
 * @ingroup BEGASEP
 * @def     BUF_LEN
 * @brief   Define @a BUF_LEN to indicate the length of the buffer to read from
 * the socket
 */
#define BUF_LEN 256
/*!
 * @ingroup BEGASEP
 * @def     BEGASEP_TRACE_ON
 * @brief   Define @a BEGASEP_TRACE_ON to turn on tracing
 */

#undef BEGASEP_TRACE_ON

#ifdef BEGASEP_TRACE_ON
   #define TRACE_LOG(fmt, args...) do{printf(fmt, ##args);}while(0)
#else
   #define TRACE_LOG(fmt, args...) ;
#endif

/*!
 * @ingroup BEGASEP
 * @enum    begasep_msg_type
 * @brief   Enum holding the types of the protocol messages. Used as states for
 * the clients as well
 */
typedef enum
{
   BEGASEP_NONE,
   BEGASEP_OPEN,
   BEGASEP_ACCEPT,
   BEGASEP_BET,
   BEGASEP_RESULT
} begasep_msg_type;

/*!
 * @ingroup BEGASEP
 * @struct  client
 * @brief   Struct holding per client information
 */
typedef struct client
{
   int sockfd; /*!< socket file decriptor */
   int state;  /*!< state of client aka last received message */
   long int betting_num; /*!< betted number */
} client;

/*!
 * @ingroup BEGASEP
 * @struct  begasep_common_header
 * @brief   Struct defining the begasep protocol header
 */
typedef struct begasep_common_header
{
   uint8_t  ver:3;
   uint8_t  len:5;
   uint8_t  type;
   uint16_t client_id;
} __attribute__((packed)) begasep_common_header;

/*!
 * @ingroup BEGASEP
 * @typedef begasep_open
 * @brief   Defining the begasep open message (same as header)
 */
typedef begasep_common_header begasep_open;

/*!
 * @ingroup BEGASEP
 * @struct  begasep_accept
 * @brief   Struct defining the begasep accept message
 */
typedef struct begasep_accept
{
   uint32_t range_lwr_end;
   uint32_t range_upr_end;
} __attribute__((packed)) begasep_accept;

/*!
 * @ingroup BEGASEP
 * @struct  begasep_bet
 * @brief   Struct defining the begasep bet message
 */
typedef struct begasep_bet
{
   uint32_t bet_num;
} begasep_bet;

/*!
 * @ingroup BEGASEP
 * @struct  begasep_result
 * @brief   Struct defining the begasep result message
 */
typedef struct begasep_result
{
   uint8_t status;
   uint32_t win_num;
} __attribute__((packed)) begasep_result;

/*!
 * @ingroup BEGASEP
 * @brief   Start the tcp server thread
 *
 * The begasep_tcp_server() function is entry point for the tcp server thread.
 * It is responsible for handling all the incoming connections and messages but
 * the result.
 */
void* begasep_tcp_server();

/*!
 * @ingroup BEGASEP
 * @brief   Start the bet server thread
 *
 * The begasep_bet_server() function is entry point for the bet thread.
 * It is responsible for handling the generation of the winning number and its
 * distribution to the clients that have placed a bet with the result message.
 */
void* begasep_bet_server();
/** @} */ /* end group BEGASEP */
