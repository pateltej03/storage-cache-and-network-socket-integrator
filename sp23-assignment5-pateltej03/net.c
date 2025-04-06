#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <err.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include "net.h"
#include "jbod.h"

/* the client socket descriptor for the connection to the server */
int cli_sd = -1;

/* attempts to read n bytes from fd; returns true on success and false on
 * failure */
// fd - file descriptor
static bool nread(int fd, int len, uint8_t *buf) {
  int num_read = 0;
  while (num_read < len) {
    int n = read(fd, &buf[num_read], len-num_read);
    if (n < 0) {
      return false;
    }
    num_read = num_read + n;
  }
  return true;
}

/* attempts to write n bytes to fd; returns true on success and false on
 * failure */
static bool nwrite(int fd, int len, uint8_t *buf) {
  int num_written = 0;
  while (num_written < len) {
    int n = write(fd, &buf[num_written], len-num_written);
    if (n < 0) {
      return false;
    }
    num_written = num_written + n;
  }
  return true;
}

/* attempts to receive a packet from fd; returns true on success and false on
 * failure */
static bool recv_packet(int fd, uint32_t *op, uint16_t *ret, uint8_t *block) {
  
  uint8_t buf[8];
  nread(fd, 8, &buf[0]);

  int len;
  memcpy(&len, &buf[0], 2);
  memcpy(op, &buf[2], 4);
  memcpy(ret, &buf[6], 2);

  len = ntohs(len);
  *op = ntohl(*op);
  *ret = ntohs(*ret);

  //if we have read the whole packet already, nothing else to do
  if (len == 8) {
    return true;
  }
  //if packet is larger (with block attached) we also need to read the block 
  else if (len == 264) {
    uint8_t temp[256];
    nread(fd, 256, &temp[0]);
    memcpy(&block[0], &temp[0], 256);
    return true;
  }
  else {
    return false;
  }
  return true;
}

/* attempts to send a packet to sd; returns true on success and false on
 * failure */
static bool send_packet(int sd, uint32_t op, uint8_t *block) {
  int opcode = op >> 26;
  int len = 0;
  //setting packet if write block, as you need to attach a block in packet
  if (opcode == JBOD_WRITE_BLOCK) {
    len = 264;
    uint8_t packet[264];
    len = htons(len);
    op = htonl(op);
    memcpy(&packet[0], &len, 2);
    memcpy(&packet[2], &op, 4);
    memcpy(&packet[8], block, 256);
    nwrite(sd, 264, &packet[0]);
  }
  //setting packet if not write block, as you dont need to attach a block in packet
  else {
    len = 8;
    uint8_t packet[8];
    len = htons(len);
    op = htonl(op);
    memcpy(&packet[0], &len, 2);
    memcpy(&packet[2], &op, 4);
    nwrite(sd, 8, &packet[0]);
  }
  return true;
}


/* attempts to connect to server and set the global cli_sd variable to the
 * socket; returns true if successful and false if not. */
bool jbod_connect(const char *ip, uint16_t port) {
  struct sockaddr_in client_address;

  //sets the fields of the server address
  client_address.sin_family = AF_INET;
  client_address.sin_port = htons(JBOD_PORT);

  //converts server address from ipv4 to network
  if (inet_aton(ip, &(client_address.sin_addr)) == 0) {
    return false;
  }

  //creating the socket
  if ((cli_sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    return false;
  }

  //connecting to the server
  if (connect(cli_sd, (struct sockaddr *) &client_address, sizeof(client_address)) == -1) {
    return false;
  }
  
  return true;
}

/* disconnects from the server and resets cli_sd */
void jbod_disconnect(void) {
  close(cli_sd);
  cli_sd = -1;
}

/* sends the JBOD operation to the server and receives and processes the
 * response. */
int jbod_client_operation(uint32_t op, uint8_t *block) {
bool v = false;
uint16_t ret;

uint32_t rop;

v = send_packet(cli_sd, op, block);
if (v == true){
  recv_packet(cli_sd, &rop, &ret, block);
}
return ret;
}
