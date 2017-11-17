#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "hash.h"

// The initial capacity of the hash table array.
#define INITIAL_CAPACITY 16

// Threshold on load factor over which we double the capacity of the table.
#define LOAD_FACTOR_THR 0.8

// A special key value we'll use to denone an element that's been removed.
char* TOMBSTONE = "__TOMBSTONE__";

/*
 * This structure is used to represent key/value pairs in the hash table.
 */
struct association {
  char* key;
  void* value;
};


struct hash {
  struct association* table;
  unsigned int capacity;
  unsigned int num_elems;
};

/*
 * Helper function to initialize a hash table's table array to a given capacity.
 */
void _hash_table_init(struct hash* hash, unsigned int capacity) {
  hash->table = malloc(capacity * sizeof(struct association));
  assert(hash->table);
  memset(hash->table, 0, capacity * sizeof(struct association));
  hash->capacity = capacity;
  hash->num_elems = 0;
}


struct hash* hash_create() {
  struct hash* hash = malloc(sizeof(struct hash));
  assert(hash);
  _hash_table_init(hash, INITIAL_CAPACITY);
  return hash;
}


void hash_free(struct hash* hash) {
  assert(hash);
  free(hash->table);
  free(hash);
}


float hash_load_factor(struct hash* hash) {
  return hash->num_elems / (float)hash->capacity;
}


/*
 * The DJB hash function: http://www.cse.yorku.ca/~oz/hash.html.
 */
unsigned int _djb_hash(char* key) {
  unsigned long hash = 5381;
  int c;
  while ((c = *key++)) {
    hash = ((hash << 5) + hash) + c;  // hash * 33 + c
  }
  return hash;
}


void _hash_resize(struct hash* hash) {

  /*
   * Remember the old table array and its capacity and re-initialize the
   * hash with a new table array with twice the capacity.
   */
  struct association* old_table = hash->table;
  unsigned int old_capacity = hash->capacity;
  _hash_table_init(hash, old_capacity * 2);

  /*
   * Loop through the old table and re-hash all the old elements via
   * hash_insert().  This will handle updating the table's size for us.  By
   * definition, this will not cause a recursive call to _hash_resize(),
   * since the new table's capacity is doubled.
   */
  for (int i = 0; i < old_capacity; i++) {

    /*
     * We can compare key to TOMBSTONE here without strcmp() because the
     * pointer value of TOMBSTONE is unique and doesn't change.
     */
    if (old_table[i].key != NULL && old_table[i].key != TOMBSTONE) {
      hash_insert(hash, old_table[i].key, old_table[i].value);
    }

  }

  free(old_table);

}


void hash_insert(struct hash* hash, char* key, void* value) {
  assert(hash);
  assert(key);

  if (hash_load_factor(hash) > LOAD_FACTOR_THR) {
    _hash_resize(hash);
  }

  // Compute a hash value for the given key and mod to convert it to an index.
  unsigned int hashval = _djb_hash(key);
  unsigned int idx = hashval % hash->capacity;

  /*
   * Perform linear probing to find the right spot to insert or update the
   * element.  We need to probe until we've found either a NULL spot or the
   * key we're looking for.  If we stop at a TOMBSTONE, we might miss the key
   * if it already exists in the table.  For this reason, we remember the
   * first TOMBSTONE location in case we need to come back to it to insert
   * a new element.
   */
  struct association* first_tombstone = NULL;
  while (hash->table[idx].key != NULL && strcmp(key, hash->table[idx].key)) {

    /*
     * We can compare key to TOMBSTONE here without strcmp() because the
     * pointer value of TOMBSTONE is unique and doesn't change.
     */
    if (first_tombstone == NULL && hash->table[idx].key == TOMBSTONE) {
      first_tombstone = &hash->table[idx];
    }

    idx = (idx + 1) % hash->capacity;

  }

  /*
   * Figure out where/whether to update.  Default to updating the first empty
   * location we foubnd.
   */
  struct association* update_loc = &hash->table[idx];
  if (value == NULL && hash->table[idx].key == NULL) {

    /*
     * If the user wants to remove an element whose key we didn't find, nothing
     * else to do.
     */
    return;

  } else if (hash->table[idx].key == NULL && first_tombstone != NULL) {

    /*
     * If the user wants to insert a value and we ran into a TOMBSTONE along
     * the way, insert there.
     */
    update_loc = first_tombstone;

  }

  /*
   * Make the update and increment or decrement the number of elements as
   * appropriate.  Insert a TOMBSTONE if we're removing an element.
   */
  update_loc->value = value;
  if (value == NULL) {
    update_loc->key = TOMBSTONE;
    hash->num_elems--;
  } else if (update_loc->key == NULL || update_loc->key == TOMBSTONE) {
    hash->num_elems++;
    update_loc->key = key;
  }
}


void* hash_get(struct hash* hash, char* key) {
  assert(hash);
  assert(key);

  // Compute a hash value for the given key and mod to convert it to an index.
  unsigned int hashval = _djb_hash(key);
  unsigned int idx = hashval % hash->capacity;

  // Perform linear probing until we've found an empty spot (NULL key).
  while (hash->table[idx].key != NULL) {

    // If we've found the element we're looking for, return its value.
    if (!strcmp(key, hash->table[idx].key)) {
      return hash->table[idx].value;
    }

    idx = (idx + 1) % hash->capacity;

  }

  // If we've reached this point, we haven't found what we're looking for.
  return NULL;
}
