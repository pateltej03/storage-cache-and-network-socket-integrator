#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "cache.h"

static cache_entry_t *cache = NULL;
int cache_size;
int clock = 0;
int num_queries = 0;
int num_hits = 0;

bool cache_exists = false;
bool cache_empty = true;

// create cache
int cache_create(int num_entries) {

  if (num_entries > 1 && num_entries < 4097 && cache_exists == false){
    cache = calloc(num_entries, sizeof(cache_entry_t));

    if (cache != NULL){
      cache_size = num_entries;
      cache_exists = true;
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

// destroy cache
int cache_destroy(void) {
  if (cache_exists == true){
    free(cache);
    cache = NULL;
    cache_size = 0;
    cache_exists = false;
    return 1;
  }
  else{
    return -1;
  }
}

// cache lookup
int cache_lookup(int disk_num, int block_num, uint8_t *buf) {

  // validitiy check
  if (cache_exists == true && cache_empty == false && disk_num > -1 && disk_num < 16 && block_num > -1 && block_num < 256){
    
    //loop through cache to search
    for (int i = 0; i < cache_size; i++){
      
      //found block
      if(cache[i].disk_num == disk_num && cache[i].block_num == block_num && cache[i].valid == true){
        
        // validity checks
        if(cache[i].block != NULL){
          
          if (!buf){
            return -1;
          }
          memcpy(buf, cache[i].block, JBOD_BLOCK_SIZE);
          num_queries++;
          num_hits++;
          clock++;
          cache[i].access_time = clock;

          return 1;
        }
        else{
          return -1;
        }
      }

      //did not find block, go to next
      else{
        num_queries++;
        clock++;
      }
    }
  }
  else{
    return -1;
  }
  return -1;
}

// cache update
void cache_update(int disk_num, int block_num, const uint8_t *buf) {
  
  // validitiy check        
  if (cache_exists == true && cache_empty == false && disk_num > -1 && disk_num < 16 && block_num > -1 && block_num < 256){
    
    //loop through cache to search
    for (int i = 0; i < cache_size; i++){
      
      //found entry
      if(cache[i].disk_num == disk_num && cache[i].block_num == block_num && cache[i].valid == true){
        
        // validity check, update and exit loop
        if(buf != NULL){
          memcpy(cache[i].block, buf, JBOD_BLOCK_SIZE);
          cache[i].access_time = clock;
          return;
        }
      }
    }

    // if entry not in cache call cache insert
    cache_insert(disk_num, block_num, buf);
  }
}

// cache insert
int cache_insert(int disk_num, int block_num, const uint8_t *buf) {
  int oldestclock = clock;
  
  //validity check
  if(cache_exists == true && disk_num > -1 && disk_num < 16 && block_num > -1 && block_num < 256 && buf != NULL){
    
    // fail if entry already in cache
    for (int i = 0; i < cache_size; i++){
      if(cache[i].disk_num == disk_num && cache[i].block_num == block_num && cache[i].valid == true){
        return -1;
      }
    }

    //loop through cache to search for empty entry
    for (int i = 0; i < cache_size; i++){
      
      //found empty entry
      if(cache[i].valid == false){
        cache[i].valid = true;
        cache[i].block_num = block_num;
        cache[i].disk_num = disk_num;
        memcpy(cache[i].block, buf, JBOD_BLOCK_SIZE);
        cache[i].access_time = clock;
        cache_empty = false;
        return 1;
      }
    }
    int tempi = 0;
    
    // loops through to find the smallest time and index
    for(int i = 0; i < cache_size; i++){
    
      // keeps updating smallest time and index
      if(cache[i].access_time < oldestclock){
        tempi = i;
      }
      
    }

    // replace smallest time entry
    cache[tempi].valid = true;
    cache[tempi].disk_num = disk_num;
    cache[tempi].block_num = block_num;
    cache[tempi].access_time = clock;
    memcpy(cache[tempi].block, buf, JBOD_BLOCK_SIZE);
    return 1;
  }
  else{
    return -1;
  }
  return -1;
}

// return true if cache enabled
bool cache_enabled(void) {
  if (cache_exists){
    return true;
  }
  else{
    return false;
  }
}

void cache_print_hit_rate(void) {
  fprintf(stderr, "Hit rate: %5.3f%%\n", 100 * (float) num_hits / num_queries);
}
