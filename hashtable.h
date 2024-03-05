#include <stdio.h>
#include <stdbool.h>

#define HT_DEFAULT_SIZE 8 // 2^3
#define HT_RESIZE_THRESHOLD 0.8

typedef int (*ht_hash)(const void* key, int seed);
typedef bool (*ht_keq)(const void* a, const void* b);

typedef struct ht_bucket_t {
    void* key;
    void* value;
    struct ht_bucket_t* next;
} ht_bucket_t;

typedef struct hash_table_t {
    ht_hash hash;                       /* hash function */
    ht_keq keq;                         /* key equality function */
    int seed;                           /* randomly generated used in hash function */
    
    ht_bucket_t** buckets;              /* bucket list */
    unsigned int size;                  /* num of buckets in the hash table */
    unsigned int count;                 /* num of <key, value> pairs in the hash table */
} hash_table_t;

typedef struct ht_enum_t {
    hash_table_t* ht;
    ht_bucket_t* curr;
    size_t index;
} ht_enum_t;

hash_table_t* ht_create(ht_hash hash, ht_keq keq, int seed);
int ht_destory(hash_table_t* hash_table);

int ht_insert(hash_table_t* hash_table, void* key, void* value);
int ht_delete(hash_table_t* hash_table, void* key);

int ht_get(hash_table_t* hash_table, void* key, void** value);

ht_enum_t* ht_create_enum(hash_table_t* hash_table);
bool ht_enum_next(ht_enum_t* en, void** key, void** value);
void ht_enum_destroy(ht_enum_t* en);

void ht_print_table(hash_table_t* hash_table);