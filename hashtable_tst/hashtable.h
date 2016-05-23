#include <stdbool.h>

typedef struct hashtable_s hashtable_t;
typedef unsigned long (*hash_fnc)(const void *) __attribute__((transaction_safe));

hashtable_t *hashtable_alloc(long init_num_bucks, hash_fnc hash);
void hashtable_free(hashtable_t *ht_ptr);
bool hashtable_insert (hashtable_t *ht_ptr, void *key, void *data);

void hashtable_print(hashtable_t *ht_ptr);
long hashtable_total_size(hashtable_t *ht_ptr);

