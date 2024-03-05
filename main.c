#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>

#include "murmur3.h"
#include "hashtable.h"

#define KEY_LENGTH 4

bool keq(const void* a, const void* b) {
    return *(int*)a == *(int*)b;
}

int hash(const void* key, int seed) {
    uint32_t hash;
    MurmurHash3_x86_32(key, KEY_LENGTH, seed, &hash);
    
    return hash;
}

/* 
    UNIT TESTS
*/

int main(int argc, char const *argv[]) {
    srand(time(NULL));

    hash_table_t* ht = ht_create(hash, keq, 42);

    int len = 128;
    int values[len];
    int keys[len];
    for(int i = 0; i < len; i++) {
        values[i] = i;
        keys[i] = i;
    }

    for(int i = 0; i < len; i++) {
        ht_insert(ht, keys + i, values + i);
    }

    ht_print_table(ht);

    ht_enum_t* en = ht_create_enum(ht);
    void* key;
    void* value;
    while(ht_enum_next(en, &key, &value)) {
        printf("<%d, %d>\n", *(int*)key, *(int*)value);
    }
    ht_enum_destroy(en);

    ht_destory(ht);

    return 0;
}