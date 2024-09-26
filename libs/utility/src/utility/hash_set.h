/* Copyright (c) 2024 Garry Whitehead
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef __UTILITY_HASH_SET_H__
#define __UTILITY_HASH_SET_H__

#include "arena.h"

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

#define HASH_SET_INIT_CAPACITY 255
#define HASH_SET_MAX_SIZE 0xffff
#define HASH_NULL 0x00
#define HASH_DELETED UINT64_MAX

typedef struct HashSet hash_set_t;

typedef uint32_t(hast_func_t)(void*, uint32_t);

typedef struct HashNode
{
    uint64_t hash;
    void* value;
    uint16_t delta[2];

} hash_set_node_t;

typedef struct HashSet
{
    uint32_t capacity;
    uint32_t size;
    arena_t* arena;
    hash_set_node_t* nodes;
    hast_func_t* hash_func;
    uint32_t value_type_size;
    uint32_t key_type_size;

    /** Private **/
    hash_set_node_t* _curr_node;
} hash_set_t;


#define HASH_SET_CREATE(key_type, val_type, arena, hash_func)                                      \
    hash_set_create(arena, hash_func, sizeof(key_type), sizeof(val_type))

#define HASH_SET_FIND(set, key)                                                                    \
    ({                                                                                             \
        __auto_type _key = (key);                                                                  \
        assert((set)->key_type_size == sizeof(*_key));                                             \
        hash_set_find(set, _key);                                                                  \
    })

#define HASH_SET_INSERT(set, key, value)                                                           \
    ({                                                                                             \
        __auto_type _key = (key);                                                                  \
        __auto_type _val = (value);                                                                \
        assert((set)->key_type_size == sizeof(*_key));                                             \
        assert((set)->value_type_size == sizeof(*_val));                                           \
        hash_set_insert(set, _key, _val);                                                          \
    })

#define HASH_SET_GET(set, key)                                                                     \
    ({                                                                                             \
        __auto_type _key = (key);                                                                  \
        assert((set)->key_type_size == sizeof(*_key));                                             \
        hash_set_get(set, _key);                                                                   \
    })

#define HASH_SET_SET(set, key, value)                                                              \
    ({                                                                                             \
        __auto_type _key = (key);                                                                  \
        __auto_type _val = (value);                                                                \
        assert((set)->key_type_size == sizeof(*_key));                                             \
        assert((set)->value_type_size == sizeof(*_val));                                           \
        hash_set_set(set, _key, value);                                                            \
    })

#define HASH_SET_ERASE(set, key)                                                                   \
    ({                                                                                             \
        __auto_type _key = (key);                                                                  \
        assert((set)->key_type_size == sizeof(*_key));                                             \
        hash_set_erase(set, _key);                                                                 \
    })


/**

 @param arena
 @param hash_func
 @param key_type_size
 @param value_type_size
 @return
 */
hash_set_t hash_set_create(
    arena_t* arena, hast_func_t* hash_func, uint32_t key_type_size, uint32_t value_type_size);

/**

 @param set
 @param key
 @param value
 @return
 */
void* hash_set_set(hash_set_t* set, void* key, void* value);

/**

 @param set
 @param key
 @return
 */
void* hash_set_get(hash_set_t* set, void* key);

/**

 @param set
 @param key
 @param value
 @return
 */
void* hash_set_insert(hash_set_t* set, void* key, void* value);

/**

 @param set
 @param key
 @return
 */
bool hash_set_find(hash_set_t* set, void* key);

/**

 @param set
 @param key
 @return
 */
void* hash_set_erase(hash_set_t* set, void* key);

uint32_t hash_set_default_hasher(void* key, uint32_t size);

void hash_set_clear(hash_set_t* set);

#endif