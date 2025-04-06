#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "mdadm.h"
#include "jbod.h"
#include "cache.h"
#include "net.h"

char MOUNTED = 'N';

uint32_t encode_op(int cmd, int disk_num, int block_num){
 uint32_t op = 0;

 op = op | (cmd << 26);
 op = op | (disk_num << 22);
 op = op | (block_num);
 return op;
}

// mount 
int mdadm_mount(void) {
  int v = 0;
  if (MOUNTED == 'N'){
    uint32_t op = encode_op(JBOD_MOUNT, 0, 0);

    v = jbod_client_operation(op, NULL);
    if (0 == v){
      MOUNTED = 'Y';
      return 1;
    }
    else{
      return -1;
    }
  }
  else{
  return -1;
  }
}

// unmount
int mdadm_unmount(void) {
  int v = 0;
  if (MOUNTED == 'Y'){
    uint32_t op = encode_op(JBOD_UNMOUNT, 0, 0);

    v = jbod_client_operation(op, NULL);
    if (v == 0){
      MOUNTED = 'N';
      return 1;
    }
    else{
      return -1;
    }
  }
  else{
  return -1;
  }
}

// mdadm read
int mdadm_read(uint32_t addr, uint32_t len, uint8_t *buf) {
  if (MOUNTED == 'Y' && len < 1025 && (addr + len ) < 0x00100001 ){
    if (buf != NULL || (buf == NULL && len == 0)){
      
      int v = 0;
      int c = 0;
      uint8_t buffer[256] = {};
      char Across = 'N';

      int disk_id = 0;
      int block_id = 0;
      int byte_id = 0;
      int byte_offset = 0;
      int bytes_left_to_read = len;
      int bytes_read = 0;

      disk_id = addr / 65536;
      block_id = (addr % 65536) / 256;
      byte_id = (addr % 65536) % 256;
      byte_offset = byte_id;
      
      uint32_t op;

      //while all bytes are not read
      while (bytes_left_to_read !=0){
          
          op = encode_op(JBOD_SEEK_TO_DISK, disk_id, 0);
          v = jbod_client_operation(op, NULL);
          if (v == 0){

              op = encode_op(JBOD_SEEK_TO_BLOCK, 0, block_id);
              v = jbod_client_operation(op, NULL);

              if (v == 0){
                c = cache_lookup(disk_id, block_id, &buffer[0]);
                //if in cache, lookup was successful
                if (c == 1){
                  v = 0;
                }
                else{
                  //if not in cache
                  op = encode_op(JBOD_READ_BLOCK, 0, 0);
                  v = jbod_client_operation(op, &buffer[0]);
                  cache_insert(disk_id, block_id, &buffer[0]);
                }
                

                if(v == 0){
                  // if all bytes are in the same block
                  if (len<(256 - byte_offset)&& (Across == 'N')){
                    memcpy(buf, buffer, len);
                    bytes_left_to_read = 0;
                    bytes_read = 0;
                  }
                  else{
                    // if complete blocks need to be read
                    if(bytes_left_to_read>(256-byte_offset)){
                      Across = 'Y';
                      memcpy(buf+bytes_read, buffer, (256-byte_offset));

                      bytes_left_to_read = bytes_left_to_read - (256-byte_offset);
                      bytes_read = bytes_read + (256-byte_offset);
                      byte_offset = 0;

                      block_id ++;
                      if(block_id>255){
                        disk_id++;
                        block_id = 0;
                      }

                    }
                    else{
                      // if this is the last block to read from
                      memcpy(buf+bytes_read, buffer, bytes_left_to_read);
                      bytes_left_to_read = 0;
                      bytes_read = 0;
                    }
                  }
                }
                else{return -1;}
              }
              else{return -1;}
          }
          else{return -1;}
      }  
    return len;
    }
  }
  else{return -1;}
  return -1;
}



// mdadm write
int mdadm_write(uint32_t addr, uint32_t len, const uint8_t *buf) {
  
  if (MOUNTED == 'Y' && len < 1025 && (addr + len ) < 0x00100001 ){
    if (buf != NULL || (buf == NULL && len == 0)){

      int v = 0;
      int c = 0;
      uint8_t buffer[256] = {};
      char Across = 'N';

      int disk_id = 0;
      int block_id = 0;
      int byte_id = 0;
      int byte_offset = 0;
      int bytes_left_to_write = len;
      int bytes_read = 0;

      disk_id = addr / 65536;
      block_id = (addr % 65536) / 256;
      byte_id = (addr % 65536) % 256;
      byte_offset = byte_id;
      uint32_t op;
      
      // while all bytes are not written
      while (bytes_left_to_write !=0){
          
          op = encode_op(JBOD_SEEK_TO_DISK, disk_id, 0);
          v = jbod_client_operation(op, NULL);
          if (v == 0){

              op = encode_op(JBOD_SEEK_TO_BLOCK, 0, block_id);
              v = jbod_client_operation(op, NULL);

              if (v == 0){

                c = cache_lookup(disk_id, block_id, &buffer[0]);
                //if in cache, lookup was successful
                if (c == 1){
                  v = 0;
                }
                else{
                  //if not in cache
                  op = encode_op(JBOD_READ_BLOCK, 0, 0);
                  v = jbod_client_operation(op, &buffer[0]);
                }

                if(v == 0){
                  // if bytes to write are all in the same block
                  if (len<(256 - byte_offset) && (Across == 'N')){
                    memcpy(buffer+byte_offset, buf, len);
                    bytes_left_to_write = 0;
                    bytes_read = 0;

                    op = encode_op(JBOD_SEEK_TO_DISK,disk_id,0);
                    jbod_client_operation(op,NULL);
                    op = encode_op(JBOD_SEEK_TO_BLOCK,0,block_id);
                    jbod_client_operation(op, NULL);
                    op = encode_op(JBOD_WRITE_BLOCK,0,0);
                    jbod_client_operation(op,&buffer[0]);
                    cache_update(disk_id, block_id, &buffer[0]);

                  }
                  else{
                    // if complete blocks need to be written
                    if(bytes_left_to_write>(256-byte_offset)){
                      Across = 'Y';
                      memcpy(buffer+byte_offset, buf+bytes_read, (256-byte_offset));

                      bytes_left_to_write = bytes_left_to_write - (256-byte_offset);
                      bytes_read = bytes_read + (256-byte_offset);
                      byte_offset = 0;

                      op = encode_op(JBOD_SEEK_TO_DISK,disk_id,0);
                      jbod_client_operation(op,NULL);
                      op = encode_op(JBOD_SEEK_TO_BLOCK,0,block_id);
                      jbod_client_operation(op, NULL);
                      op = encode_op(JBOD_WRITE_BLOCK,0,0);
                      jbod_client_operation(op,&buffer[0]);
                      cache_update(disk_id, block_id, &buffer[0]);
                      

                      block_id ++;
                      if(block_id>255){
                        disk_id++;
                        block_id = 0;
                      }

                    }
                    else{
                      // if this is the last block to write into
                      memcpy(buffer+byte_offset, buf+bytes_read, bytes_left_to_write);
                      bytes_left_to_write = 0;
                      bytes_read = 0;

                      op = encode_op(JBOD_SEEK_TO_DISK,disk_id,0);
                      jbod_client_operation(op,NULL);
                      op = encode_op(JBOD_SEEK_TO_BLOCK,0,block_id);
                      jbod_client_operation(op, NULL);
                      op = encode_op(JBOD_WRITE_BLOCK,0,0);
                      jbod_client_operation(op,&buffer[0]);
                      cache_update(disk_id, block_id, &buffer[0]);
                    }
                  }
                }
                else{return -1;}
              }
              else{return -1;}
          }
          else{return -1;}
      }  
      
      return len;
    }
    else{return -1;}
  }
  
  else{return -1;}
}
