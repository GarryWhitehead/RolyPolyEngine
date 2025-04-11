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

#include "hash_set.h"

#include <assert.h>
#include <string.h>

#define FNV_OFFSET 14695981039346656037UL
#define FNV_PRIME 1099511628211UL

uint32_t _index_from_hash(hash_set_t* set, uint64_t hash)
{
    return hash & (uint64_t)(set->capacity - 1);
}

void* _copy_value_to_set(hash_set_t* set, void* value)
{
    void* new_val = arena_alloc(
        set->arena, set->value_type_size, set->value_type_size, 1, ARENA_NONZERO_MEMORY);
    memcpy(new_val, value, set->value_type_size);
    return new_val;
}

void _find(hash_set_t* set, void* key, uint64_t hash)
{
    set->_curr_node = NULL;

    uint32_t idx = _index_from_hash(set, hash);

    hash_set_node_t* node = &set->nodes[idx];
    if (node->hash == hash)
    {
        set->_curr_node = node;
        return;
    }

    uint16_t delta = node->delta[0];

    while (delta)
    {
        idx += delta;
        if (set->nodes[idx].hash == hash)
        {
            set->_curr_node = &set->nodes[idx];
            return;
        }
        delta = set->nodes[idx].delta[1];
    }

    set->_curr_node = NULL;
}

enum InsertResult
{
    InsertResult_Inserted,
    InsertResult_OutOfSpace,
    InsertResult_Already
};
enum InsertResult _insert(hash_set_t* set, uint64_t hash, void* value, void** new_value)
{
    assert(set);
    assert(value);

    *new_value = NULL;
    uint32_t idx = _index_from_hash(set, hash);
    assert(idx < set->capacity);

    hash_set_node_t* node = &set->nodes[idx];
    if (node->hash == hash)
    {
        return InsertResult_Already;
    }
    else if (node->hash == HASH_NULL || node->hash == HASH_DELETED)
    {
        node->hash = hash;
        node->value = _copy_value_to_set(set, value);
        *new_value = node->value;
        ++set->size;
        return InsertResult_Inserted;
    }

    uint16_t* prev_delta = &node->delta[0];
    uint16_t delta = *prev_delta;

    while (delta)
    {
        idx += delta;
        if (set->nodes[idx].hash == hash)
        {
            return InsertResult_Already;
        }
        prev_delta = &set->nodes[idx].delta[1];
        delta = *prev_delta;
    }

    uint32_t begin_idx = idx;
    int remaining = (int)set->capacity - (int)idx;
    assert(remaining >= 0);
    // Not found via leap-frogging, so revert to linear probing.
    for (; idx < set->capacity; ++idx)
    {
        if (set->nodes[idx].hash == HASH_NULL || set->nodes[idx].hash == HASH_DELETED)
        {
            set->nodes[idx].hash = hash;
            set->nodes[idx].value = _copy_value_to_set(set, value);
            *new_value = set->nodes[idx].value;
            *prev_delta = idx - begin_idx;
            ++set->size;
            return InsertResult_Inserted;
        }
    }

    // We are out of space so signal a resize is required.
    return InsertResult_OutOfSpace;
}

enum ResizeResult
{
    ResizeResult_Ok,
    ResizeResult_NoMemory,
    ResizeResult_OutOfSpace
};
enum ResizeResult _set_resize(hash_set_t* set)
{
    uint32_t old_capacity = set->capacity;
    uint32_t new_capacity = set->capacity * 2;
    hash_set_node_t* old_nodes = set->nodes;

    if (new_capacity < set->capacity)
    {
        return ResizeResult_Ok;
    }
    hash_set_node_t* new_nodes =
        ARENA_MAKE_ARRAY(set->arena, hash_set_node_t, new_capacity, ARENA_ZERO_MEMORY);
    if (!new_nodes)
    {
        return ResizeResult_NoMemory;
    }

    set->nodes = new_nodes;
    set->size = 0;
    set->capacity = new_capacity;

    for (uint32_t i = 0; i < old_capacity; ++i)
    {
        hash_set_node_t* old_node = &old_nodes[i];
        if (old_node->hash != HASH_DELETED && old_node->hash != HASH_NULL)
        {
            void* out;
            enum InsertResult res = _insert(set, old_node->hash, old_node->value, &out);
            assert(res != InsertResult_Already);
            if (res == InsertResult_OutOfSpace)
            {
                return ResizeResult_OutOfSpace;
            }
        }
    }
    return ResizeResult_Ok;
}

/** Public functions **/

hash_set_t hash_set_create(
    arena_t* arena, hast_func_t* hash_func, uint32_t key_type_size, uint32_t value_type_size)
{
    assert(arena);
    assert(hash_func);
    assert(key_type_size > 0);
    assert(value_type_size > 0);

    hash_set_t set;
    set.size = 0;
    set.capacity = HASH_SET_INIT_CAPACITY;
    set.arena = arena;
    set.hash_func = hash_func;
    set.value_type_size = value_type_size;
    set.key_type_size = key_type_size;
    set._curr_node = NULL;

    // Allocate the initial entries.
    set.nodes = ARENA_MAKE_ARRAY(arena, hash_set_node_t, set.capacity, ARENA_ZERO_MEMORY);
    return set;
}

void* hash_set_get(hash_set_t* set, void* key)
{
    assert(key);
    uint64_t hash = set->hash_func(key, set->key_type_size, 0);
    _find(set, key, hash);
    return !set->_curr_node ? NULL : set->_curr_node->value;
}

bool hash_set_find(hash_set_t* set, void* key)
{
    assert(key);
    uint64_t hash = set->hash_func(key, set->key_type_size, 0);
    _find(set, key, hash);
    return !set->_curr_node ? false : true;
}

void* hash_set_set(hash_set_t* set, void* key, void* value)
{
    assert(key);
    assert(value);

    uint64_t hash = set->hash_func(key, set->key_type_size, 0);
    _find(set, key, hash);
    if (set->_curr_node)
    {
        void* old_value = set->_curr_node->value;
        set->_curr_node->value = _copy_value_to_set(set, value);
        return old_value;
    }
    // Key not in current hash set so insert.
    void* out = NULL;
    for (;;)
    {
        enum InsertResult res = _insert(set, hash, value, &out);
        if (res != InsertResult_OutOfSpace)
        {
            return out;
        }
        enum ResizeResult r = _set_resize(set);
        if (r == ResizeResult_NoMemory)
        {
            return NULL;
        }
    }
}

void* hash_set_insert(hash_set_t* set, void* key, void* value)
{
    assert(value);
    assert(key);

    uint64_t hash = set->hash_func(key, set->key_type_size, 0);
    void* out = NULL;
    for (;;)
    {
        enum InsertResult res = _insert(set, hash, value, &out);
        if (res != InsertResult_OutOfSpace)
        {
            return out;
        }
        enum ResizeResult r = _set_resize(set);
        if (r == ResizeResult_NoMemory)
        {
            return NULL;
        }
    }
}

void* hash_set_erase(hash_set_t* set, void* key)
{
    assert(key);
    uint64_t hash = set->hash_func(key, set->key_type_size, 0);
    _find(set, key, hash);
    assert(set->_curr_node);

    --set->size;
    void* old_value = set->_curr_node->value;
    set->_curr_node->hash = HASH_DELETED;
    return old_value;
}

uint32_t hash_set_default_hasher(void* key, uint32_t size)
{
    uint64_t hash = FNV_OFFSET;
    const char* char_key = (const char*)key;
    for (const char* k = char_key; *k; ++k)
    {
        hash ^= (uint64_t)(uint8_t)(*k);
        hash ^= FNV_PRIME;
    }
    return (uint32_t)hash;
}

void hash_set_clear(hash_set_t* set)
{
    assert(set);
    memset(set->nodes, 0, sizeof(struct HashNode) * set->size);
    set->size = 0;
    set->_curr_node = NULL;
}

uint32_t hash_set_find_next(hash_set_t* set, uint32_t idx)
{
    assert(set);
    if (!set->size)
    {
        return 0;
    }
    // Advance the index along by one item.
    uint32_t curr_idx = idx;
    while (curr_idx < set->capacity)
    {
        if (set->nodes[curr_idx].hash != HASH_NULL && set->nodes[curr_idx].hash != HASH_DELETED)
        {
            assert(set->nodes[curr_idx].value);
            return curr_idx;
        }
        curr_idx++;
    }
    return set->capacity;
}

hash_set_iterator_t hash_set_iter_create(hash_set_t* set)
{
    assert(set);
    hash_set_iterator_t it;
    it.curr_idx = 0;
    it.set = set;
    return it;
}

void* hash_set_iter_next(hash_set_iterator_t* it)
{
    assert(it);
    uint32_t idx = hash_set_find_next(it->set, it->curr_idx);
    if (idx == it->set->capacity)
    {
        return NULL;
    }
    it->curr_idx = idx + 1;
    return it->set->nodes[idx].value;
}

hash_set_iterator_t hash_set_iter_erase(hash_set_iterator_t* it)
{
    assert(it);
    hash_set_t* set = it->set;
    // We need to decrement the current index as the next function stores the current index + 1 to
    // avoid pulling in the same bucket.
    hash_set_node_t* node = &set->nodes[it->curr_idx - 1];

    --set->size;
    node->hash = HASH_DELETED;

    // Return new iterator pointing at the next item in the set.
    hash_set_iterator_t new_it;
    new_it.set = set;
    new_it.curr_idx = hash_set_find_next(set, it->curr_idx + 1);
    return new_it;
}
