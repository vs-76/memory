// Copyright (C) 2015 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include "detail/free_list.hpp"
#include "detail/small_free_list.hpp"

#include <algorithm>
#include <catch.hpp>
#include <random>
#include <vector>

using namespace foonathan::memory;
using namespace detail;

template <class FreeList>
void use_list_node(FreeList &list)
{
    std::vector<void*> ptrs;
    auto capacity = list.capacity();
    for (std::size_t i = 0u; i != capacity; ++i)
        ptrs.push_back(list.allocate());
    REQUIRE(list.capacity() == 0u);
    REQUIRE(list.empty());

    std::shuffle(ptrs.begin(), ptrs.end(), std::mt19937{});

    for (auto p : ptrs)
        list.deallocate(p);
    REQUIRE(list.capacity() == capacity);
    REQUIRE(!list.empty());
}

template <class FreeList>
void check_list(FreeList &list, void *memory, std::size_t size)
{
    auto old_cap = list.capacity();

    list.insert(memory, size);
    REQUIRE(!list.empty());
    REQUIRE(list.capacity() <= old_cap + size / list.node_size());

    old_cap = list.capacity();

    auto node = list.allocate();
    REQUIRE(list.capacity() == old_cap - 1);

    list.deallocate(node);
    REQUIRE(list.capacity() == old_cap);

    use_list_node(list);
}

template <class FreeList>
void check_move(FreeList &list)
{
    char memory[1024];
    list.insert(memory, 1024);

    auto ptr = list.allocate();
    auto capacity = list.capacity();

    auto list2 = std::move(list);
    REQUIRE(list.empty());
    REQUIRE(list.capacity() == 0u);
    REQUIRE(!list2.empty());
    REQUIRE(list2.capacity() == capacity);

    list2.deallocate(ptr);

    char memory2[1024];
    list.insert(memory2, 1024);
    REQUIRE(!list.empty());
    REQUIRE(list.capacity() <= 1024 / list.node_size());

    ptr = list.allocate();
    list.deallocate(ptr);

    ptr = list2.allocate();

    list = std::move(list2);
    REQUIRE(list2.empty());
    REQUIRE(list2.capacity() == 0u);
    REQUIRE(!list.empty());
    REQUIRE(list.capacity() == capacity);

    list.deallocate(ptr);
}

TEST_CASE("free_memory_list", "[detail][pool]")
{
    free_memory_list list(4);
    REQUIRE(list.empty());
    REQUIRE(list.node_size() >= 4);
    REQUIRE(list.capacity() == 0u);

    SECTION("normal insert")
    {
        char memory[1024];
        check_list(list, memory, 1024);
    }
    SECTION("uneven insert")
    {
        char memory[1023]; // not dividable
        check_list(list, memory, 1023);
    }
    SECTION("multiple insert")
    {
        char a[1024], b[100], c[1337];
        check_list(list, a, 1024);
        check_list(list, b, 100);
        check_list(list, c, 1337);
    }
    check_move(list);
}

void use_list_array(ordered_free_memory_list &list)
{
    // just hoping to catch segfaults

    auto array = list.allocate(3, list.node_size());
    auto array2 = list.allocate(2, 3);
    auto node = list.allocate();

    list.deallocate(array2, 2, 3);
    list.deallocate(node);

    array2 = list.allocate(4, 10);

    list.deallocate(array, 3, list.node_size());

    node = list.allocate();
    list.deallocate(node);

    list.deallocate(array2, 4, 10);
}

TEST_CASE("ordered_free_memory_list", "[detail][pool]")
{
    ordered_free_memory_list list(4);
    REQUIRE(list.empty());
    REQUIRE(list.node_size() >= 4);
    REQUIRE(list.capacity() == 0u);

    SECTION("normal insert")
    {
        char memory[1024];
        check_list(list, memory, 1024);
        use_list_array(list);
    }
    SECTION("uneven insert")
    {
        char memory[1023]; // not dividable
        check_list(list, memory, 1023);
        use_list_array(list);
    }
    SECTION("multiple insert")
    {
        char a[1024], b[100], c[1337];
        check_list(list, a, 1024);
        use_list_array(list);
        check_list(list, b, 100);
        use_list_array(list);
        check_list(list, c, 1337);
        use_list_array(list);
    }
    check_move(list);
}

TEST_CASE("small_free_memory_list", "[detail][pool]")
{
    small_free_memory_list list(4);
    REQUIRE(list.empty());
    REQUIRE(list.node_size() == 4);
    REQUIRE(list.capacity() == 0u);

    SECTION("normal insert")
    {
        char memory[1024];
        check_list(list, memory, 1024);
    }
    SECTION("uneven insert")
    {
        char memory[1023]; // not dividable
        check_list(list, memory, 1023);
    }
    SECTION("big insert")
    {
        char memory[4096]; // should use multiple chunks
        check_list(list, memory, 4096);
    }
    SECTION("multiple insert")
    {
        char a[1024], b[100], c[1337];
        check_list(list, a, 1024);
        check_list(list, b, 100);
        check_list(list, c, 1337);
    }
    check_move(list);
}
