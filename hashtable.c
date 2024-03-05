#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <limits.h>

#include "hashtable.h"
#include "murmur3.h"

struct hash_table_t* ht_create(ht_hash hash, ht_keq keq, int seed) {
    hash_table_t* ht = malloc(sizeof(hash_table_t));
    ht->hash = hash;
    ht->keq = keq;
    ht->seed = seed;

    ht->size = HT_DEFAULT_SIZE;
    ht->count = 0;

    ht->buckets = calloc(HT_DEFAULT_SIZE, sizeof(ht_bucket_t*));

    return ht;
}

void _destory_buckets(hash_table_t* hash_table) {
    for(size_t i = 0; i < hash_table->size; i++) {
        ht_bucket_t* curr = hash_table->buckets[i];
        ht_bucket_t* last = NULL;
        while(curr != NULL) {
            last = curr;
            curr = curr->next;
            free(last);
        }
    }
}

int ht_destory(hash_table_t* hash_table) {
    if(hash_table == NULL) {
        return 1;
    }

    _destory_buckets(hash_table);
    free(hash_table->buckets);
    free(hash_table);

    return 0;
}

int _ht_index(hash_table_t* hash_table, void* key) {
    int hash = hash_table->hash(key, hash_table->seed);
    /* size is always power of 2, so this is a fast way to calculate modulo */
    int index = hash & (hash_table->size - 1);
    
    return index;
}

void _ht_insert_bucket(hash_table_t* hash_table, ht_bucket_t* bucket) {
    int index = _ht_index(hash_table, bucket->key);
    ht_bucket_t* curr_bucket = hash_table->buckets[index];

    if(curr_bucket == NULL) {
        hash_table->buckets[index] = bucket;

    } else {
        while(curr_bucket != NULL) {
            if(hash_table->keq(curr_bucket->key, bucket->key)) {
                curr_bucket->value = bucket->value;
                break;
            }

            if(curr_bucket->next == NULL) {
                curr_bucket->next = bucket;
                break;
            }

            curr_bucket = curr_bucket->next;
        }
    }

    hash_table->count++;
}

void _ht_resize(hash_table_t* hash_table) {
    unsigned int new_size = 0;

    if(hash_table->count >= (unsigned int)(hash_table->size * HT_RESIZE_THRESHOLD)) {
        if(hash_table->size >= INT_MAX)
            return;
        
        new_size = hash_table->size * 2;
    } else if(hash_table->count <= (unsigned int)(hash_table->size * (1.0 - HT_RESIZE_THRESHOLD))) {
        unsigned int half_size = hash_table->size / 2;
        if(half_size < HT_DEFAULT_SIZE)
            return;
        
        new_size = half_size;
    } else {
        return;
    }
    
    unsigned int old_size = hash_table->size;
    ht_bucket_t** old_buckets = hash_table->buckets;
    ht_bucket_t** new_buckets = calloc(new_size, sizeof(ht_bucket_t*));

    hash_table->buckets = new_buckets;
    hash_table->size = new_size;
    hash_table->count = 0;

    for(size_t i = 0; i < old_size; i++) {
        ht_bucket_t* curr = old_buckets[i];
        ht_bucket_t* next = NULL;
        while(curr != NULL) {
            next = curr->next;
            
            curr->next = NULL;
            _ht_insert_bucket(hash_table, curr);

            curr = next;
        }
    }

    free(old_buckets);
}

ht_bucket_t* _create_bucket(void* key, void* value) {
    ht_bucket_t* bucket = malloc(sizeof(ht_bucket_t));
    bucket->key = key;
    bucket->value = value;
    bucket->next = NULL;

    return bucket;
}

int ht_insert(hash_table_t* hash_table, void* key, void* value) {
    if(hash_table == NULL)
        return 1;

    ht_bucket_t* bucket = _create_bucket(key, value);
    if(bucket == NULL)
        return 1;
    
    _ht_insert_bucket(hash_table, bucket);
    
    _ht_resize(hash_table);
    
    return 0;
}

int ht_delete(hash_table_t* hash_table, void* key) {
    if(hash_table == NULL)
        return 1;

    int index = _ht_index(hash_table, key);
    ht_bucket_t* last = NULL;
    ht_bucket_t* curr = hash_table->buckets[index];

    if(curr == NULL) /* key not in hash table */
        return 0;

    bool isKeyMatched = hash_table->keq(curr->key, key);
    if(isKeyMatched) { /* key is in first bucket */
        hash_table->buckets[index] = curr->next;
        free(curr);
        hash_table->count--;
        _ht_resize(hash_table);
        return 0;
    }

    /* search buckets until key is found or no more buckets */
    while(curr->next != NULL && !isKeyMatched) {
        last = curr;
        curr = curr->next;
        isKeyMatched = hash_table->keq(curr->key, key);
    }

    /* curr is either the last bucket, or the key was found */
    if(curr->next == NULL) {
        if(isKeyMatched) { /* key is in last bucket */
            last->next = NULL;
            free(curr);
            hash_table->count--;
        }
        
        /* key was either in last bucket, or wasn't found */
        _ht_resize(hash_table);
        return 0;
    } 
    
    /* key is in a middle bucket */
    last->next = curr->next;
    free(curr);
    hash_table->count--;
    _ht_resize(hash_table);

    return 0;
}

int ht_get(hash_table_t* hash_table, void* key, void** value) {
    if(hash_table == NULL)
        return 1;

    int index = _ht_index(hash_table, key);
    ht_bucket_t* curr = hash_table->buckets[index];

    while(curr != NULL && !hash_table->keq(curr->key, key)) {
        curr = curr->next;
    }

    if(curr == NULL) {
        *value = NULL;
        return 0;
    }

    *value = curr->value;
    return 0;
}

ht_enum_t* ht_create_enum(hash_table_t* hash_table) {
    if(hash_table == NULL)
        return NULL;

    ht_enum_t* en = malloc(sizeof(ht_enum_t));
    en->ht = hash_table;
    en->curr = hash_table->buckets[0];
    en->index = 0;

    return en;
}

bool ht_enum_next(ht_enum_t* en, void** key, void** value) {
    if(en == NULL)
        return false;
    
    while(en->curr == NULL) {
        if(en->index >= en->ht->size) {
            return false;
        }

        en->index++;
        en->curr = en->ht->buckets[en->index];
    }

    /* curr is not NULL */
    *key = en->curr->key;
    *value = en->curr->value;

    en->curr = en->curr->next;
    
    return true;
}

void ht_enum_destroy(ht_enum_t* enumeration) {
    if(enumeration == NULL)
        return;

    free(enumeration);
}

void ht_print_table(hash_table_t* hash_table) {
    printf("-------------------- HashTable ------------------\n");
    printf("size:\t%d\n", hash_table->size);
    printf("count:\t%d\n", hash_table->count);
    
    printf("-------------------------------------------------\n");

    for(size_t i = 0; i < hash_table->size; i++) {
        ht_bucket_t* bucket = hash_table->buckets[i];
        
        printf("%ld: ", i);
        
        if(bucket == NULL) {
            printf("-\n");
            continue;
        }

        char linked = 0;
        while(bucket != NULL) {
            if(linked) {
                printf(" --> ");
            }
            linked = 1;

            printf("<%d, %d>", *(int *)bucket->key, *(int *)bucket->value);
            
            bucket = bucket->next;
        }

        printf("\n");
    }
}