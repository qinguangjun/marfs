#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "hash_table.h"
/**
 * Compute the hash of key.
 */
unsigned long polyhash(const char *string) {
  const int salt = 33;
  char c;
  unsigned long hash = *string++;
  while((c = *string++))
    hash = salt * hash + c;
  return hash;
}

/**
 * Make a new ht_entry_t with the given key and value. (always
 * succeeds, or fails by exiting the program).
 */
ht_entry_t *new_ht_entry(const char *key, int value) {
  ht_entry_t *entry = calloc(1, sizeof (ht_entry_t));
  if(entry == NULL) {
    perror("new_ht_entry - calloc()");
    exit(-1);
  }
  entry->key = strdup(key);
  if(entry->key == NULL) {
    perror("new_ht_entry - strdup()");
    exit(-1);
  }
  entry->value = value;
  entry->payload = NULL;

  return entry;
}

/**
 * Initialize the hash table.
 *
 * @param ht    a pointer to the hash_table_t to initialize
 * @param size  the size of the hash table
 *
 * @return NULL if initialization failed. Otherwise non-NULL.
 */
void *ht_init(hash_table_t *ht, unsigned int size) {
  ht->table = calloc(size, sizeof (ht_entry_t));
  ht->size = size;
  ht->num_elements = 0;
  return ht->table;
}

/**
 * Lookup a key in the hash table.
 *
 * @param ht  the table to search
 * @param key the key to search for.
 *
 * @return the entry value if the key was found. 0 if not.
 */
int ht_lookup(hash_table_t *ht, const char* key) {
  unsigned long hash = polyhash(key);
  ht_entry_t *entry = ht->table[hash % ht->size];
  while(entry) {
    if(!strcmp(entry->key, key)) {
      return entry->value;
    }
    entry = entry->next;
  }
  return 0;
}

/**
 * Lookup a key in the hash table and return the value of its payload
 *
 * @param ht   the table to search
 * @param key  the key to search for
 *
 * @return the payload of the matching entry, or NULL if no match was found
 */
void* ht_retrieve(hash_table_t* ht, const char* key) {
  unsigned long hash = polyhash(key);
  ht_entry_t *entry = ht->table[hash % ht->size];
  while(entry) {
    if(!strcmp(entry->key, key)) {
      return entry->payload;
    }
    entry = entry->next;
  }
  return NULL;
}

/**
 * Insert an entry into the hash table.
 *
 * @param ht  the hash table to insert in
 * @param key the key to insert
 */
void ht_insert(hash_table_t *ht, const char* key) {
  unsigned long hash = polyhash(key);

  if(!ht->table[hash % ht->size]) {
    ht->table[hash % ht->size] = new_ht_entry(key, 1);
    ht->num_elements++;
  }
  else if(!strcmp(ht->table[hash % ht->size]->key, key)) {
    ht->table[hash % ht->size]->value++;
    return;
  }
  else {
    ht_entry_t *e = ht->table[hash % ht->size];
    while(e->next) {
      if(!strcmp(e->key, key)) {
        e->value++;
        return;
      }
      e = e->next;
    }
    e->next = new_ht_entry(key, 1);
    ht->num_elements++;
  }
}

/**
 * Insert an entry and payload into the hash table.
 *
 * @param ht        the hash table to insert in
 * @param key       the key to insert
 * @param payload   payload to attatch to entry (this value will replace any original payload)
 * @param ins_func  function to be called on new+old payload values when a matching entry in encountered
 */
void ht_insert_payload(hash_table_t* ht, const char* key, void* payload, void (*ins_func) (void* new, void* old) ) {
  unsigned long hash = polyhash(key);

  if(!ht->table[hash % ht->size]) {
    ht->table[hash % ht->size] = new_ht_entry(key, 1);
    ht->table[hash % ht->size]->payload = payload;
    ht->num_elements++;
  }
  else if(!strcmp(ht->table[hash % ht->size]->key, key)) {
    ht->table[hash % ht->size]->value++;
    ins_func(payload, ht->table[hash % ht->size]->payload);
    ht->table[hash % ht->size]->payload = payload;
    return;
  }
  else {
    ht_entry_t *e = ht->table[hash % ht->size];
    while(e->next) {
      if(!strcmp(e->key, key)) {
        e->value++;
        ins_func( payload, e->payload);
        e->payload = payload;
        return;
      }
      e = e->next;
    }
    e->next = new_ht_entry(key, 1);
    e->next->payload = payload;
    ht->num_elements++;
  }
}

/**
 * Traverses the hash table, returning the first non-NULL entry encountered.
 *
 * @param ht      the hash table to traverse
 * @param ht_pos  the previous entry returned by a traversal, or NULL if starting at the beginning
 *
 * @return the next entry encountered, or NULL if the end of the table has been reached
 */
ht_entry_t* ht_traverse( hash_table_t* ht, ht_entry_t* ht_pos ) {
  if ( ht->size == 0  ||  ht_pos == ht->table[ht->size - 1] ) {
    return NULL;
  }

  if( ht_pos == NULL ){
    ht_pos = ht->table[0];
  }
  else {
    ht_pos++;
  }

  while( ht_pos != ht->table[ht->size - 1] ) {
    if( ht_pos ) {
      return ht_pos;
    }
    ht_pos++;
  }

  return ht_pos;
}

