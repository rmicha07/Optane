#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <libpmemobj.h>

#define LAYOUT_NAME "pmcache"

#define MAX_KEY_SIZE 8
#define MAX_VALUE_SIZE 128
#define CACHE_SIZE 1024

/*
 * Describes a persistent cache operation.
 *
 * type can be any of 'P', 'G', 'D' for operations put, get, delete, respectively.
 */
struct Operation {
  char type;
  char key[MAX_KEY_SIZE];
  char value[MAX_VALUE_SIZE];
};

/*
 * Represents a persistent-cache slot holding a key-value pair.
 */
struct KeyValueSlot {
  char valid;
  char key[MAX_KEY_SIZE];
  char value[MAX_VALUE_SIZE];
};

/*
 * Represents a persistent cache as an array of key-value slots
 */
struct PmCacheRoot {
  struct KeyValueSlot slots[CACHE_SIZE];
};

/*
 * Parses next operation from file stream fin and stores operation metadata
 * into operation op.
 */
static int ParseNext(FILE* fin, int max_key_size, int max_value_size, struct Operation* op)
{
  char buf[128];
  if (fscanf(fin, "%1s", buf) == EOF) {
    return 1;
  }
  op->type = buf[0];

  sprintf(buf, "%%%ds", max_key_size);
  fscanf(fin, buf, op->key);

  if (op->type == 'P') {
    sprintf(buf, "%%%ds", max_value_size);
    fscanf(fin, buf, op->value);
  }
  return 0;
}

/*
 * Checks if file exists.
 */
static inline int
CheckFileExists(char const *file)
{
        return access(file, F_OK);
}


/*
 * Returns a hash for input string str.
 */
unsigned long hash(char *str)
{
    unsigned long hash = 5381;
    int c;

    while ((c = *str++))
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash;
}

/*
 * Returns slot number that corresponds to the specified key.
 */
unsigned long SlotNumber(char* key)
{
  // TODO
 return hash(key)%CACHE_SIZE;

}

/*
 * Associates the specified value with the specified key in the persistent cache.
 *
 * If the key already exists, then it overwrites its value with the new value.
 * Returns zero on success.
 */
static int Put(PMEMobjpool *pop, struct PmCacheRoot* root, char* key, char* value){
  struct KeyValueSlot* slot = &root->slots[SlotNumber(key)];
  // TODO
  // printf("In put\n");
  slot->valid='1';
  pmemobj_persist(pop,&slot->valid,1);
  //printf("Valid ok");
  pmemobj_memcpy_persist(pop,slot->key,key,MAX_KEY_SIZE);
  //printf("Valid key ");
  pmemobj_memcpy_persist(pop,slot->value,value,MAX_VALUE_SIZE);
  printf("Wrote %c %s %s in memory \n",slot->valid,slot->key,slot->value);

  /*  memcpy(slot->valid,"1",1);
  memcpy(slot->key,key,MAX_KEY_SIZE);
  memcpy(slot->value,value,MAX_VALUE_SIZE);
  pmemobj_persist(pop,root,sizeof(root));
*/

  printf("Put done");
  return 0;
}

/*
 * Returns the value to which the specified key is mapped.
 *
 * Returns zero on success.
 */
static int Get(PMEMobjpool *pop, struct PmCacheRoot* root, char* key, char* value) {
  struct KeyValueSlot* slot = &root->slots[SlotNumber(key)];
  //printf("b4 cmp\n");
  // TODO
  if (slot->valid=='1'){
    printf("EXISTS!\n");
    memcpy(value,slot->value,MAX_VALUE_SIZE);
    printf("Get ok\n");
  return 0;
      //}
    }

printf("DOES NOT EXIST\n");
return 1;
}

/*
 * Removes the mapping for the specified key from the persistent cache if present.
 *
 * Returns zero on success.
 */
static int Del(PMEMobjpool *pop, struct PmCacheRoot* root, char* key) {
  struct KeyValueSlot* slot = &root->slots[SlotNumber(key)];
  // TODO
  slot->valid='0';
  //printf("Valid 0 done");
  pmemobj_persist(pop,root,sizeof(root));
  return 0;

  return 1;
 }


/*
 * Reads standard input line by line and executes corresponding operation
 * against the persistent cache.
 */
int main(int argc, char* argv[])
{

struct Operation op;

struct KeyValueSlot* slot;

//printf("Operation Size:%d \n KeyValueSlot: %d\n\n",sizeof(op),sizeof(slot));


  if (argc != 2) {
    fprintf(stderr, "usage: %s pool-file-name\n", argv[0]);
    fprintf(stderr, "\n");
    fprintf(stderr, "Reads standard input line by line and executes corresponding operation against\nthe persistent cache.\n");
    return 1;
  }

  const char *path = argv[1];

        static PMEMobjpool *pop = NULL;

  //pop=pmemobj_create(argv[1], LAYOUT_NAME, PMEMOBJ_MIN_POOL, 0666);

        if (CheckFileExists(path) != 0) {
            pop=pmemobj_create(argv[1], LAYOUT_NAME, PMEMOBJ_MIN_POOL, 0666);

        printf("created memory\n");
        } else {
    // TODO
        pop=pmemobj_open(argv[1],LAYOUT_NAME);

   printf("Memory exists\n");

        }
  //printf("BRpoint 1\n");

  // TODO
  // PMEMoid root =
        // struct PmCacheRoot *rootp = ...

PMEMoid root = pmemobj_root(pop, sizeof (struct PmCacheRoot));
struct PmCacheRoot *rootp = pmemobj_direct(root);

 // printf("BRpoint 2\n");

  while (ParseNext(stdin, MAX_KEY_SIZE-1, MAX_VALUE_SIZE-1, &op) == 0) {
    switch (op.type) {
      case 'P': {
        if (Put(pop, rootp, op.key, op.value) == 0) {
          printf("PUT: SUCCESS (key '%s' maps to '%s')\n", op.key, op.value);
        } else {
          printf("PUT: FAILED (cannot put key '%s')\n", op.key);
        }
        break;
      }
      case 'G': {
        char value[MAX_VALUE_SIZE];
        if (Get(pop, rootp, op.key, value) == 0) {
          printf("GET: SUCCESS (key '%s' maps to '%s')\n", op.key, value);
        } else {
          printf("GET: FAILED (key '%s' not found)\n", op.key);
        }
        break;
      }
      case 'D': {
        if (Del(pop, rootp, op.key) == 0) {
          printf("DEL: SUCCESS (removed key '%s')\n", op.key);
        } else {
          printf("DEL: FAILED (cannot remove key '%s')\n", op.key);
        }
        break;
      }
      default:
        fprintf(stderr, "Unknown operation type\n");
        exit(1);
    }
  }
  abort();
  return 0;
}