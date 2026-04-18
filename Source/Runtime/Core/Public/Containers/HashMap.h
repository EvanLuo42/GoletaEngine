#pragma once

/// @file
/// @brief HashMap<K, V>: hash table with Rust-shaped API.
/// @note  v0 implementation proxies std::unordered_map through an allocator adapter.

#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <unordered_map>
#include <utility>

#include "Memory/Allocator.h"

namespace goleta
{

/// @brief Hash table mapping K to V.
/// @tparam K    Key type; must satisfy std::hash / std::equal_to unless Hash / Eq are overridden.
/// @tparam V    Value type.
/// @tparam Hash Hash functor; defaults to std::hash<K>.
/// @tparam Eq   Key equality; defaults to std::equal_to<K>.
/// @tparam A    Allocator type; defaults to goleta::allocators::Global.
template <class K, class V, class Hash = std::hash<K>, class Eq = std::equal_to<K>, class A = allocators::Global>
class HashMap
{
private:
    using Pair = std::pair<const K, V>;
    using AllocAdapter = detail::StdAllocatorAdapter<Pair, A>;
    using StdImpl = std::unordered_map<K, V, Hash, Eq, AllocAdapter>;
    StdImpl Inner;

public:
    using KeyType = K;
    using MappedType = V;
    using ValueType = Pair;
    using Hasher = Hash;
    using KeyEqual = Eq;
    using AllocatorType = A;
    using SizeType = std::size_t;
    using Iterator = typename StdImpl::iterator;
    using ConstIterator = typename StdImpl::const_iterator;

    class Entry;

    /// @brief Construct an empty HashMap with the default allocator.
    HashMap() = default;

    /// @brief Construct an empty HashMap with the given allocator.
    explicit HashMap(A Alloc)
        : Inner(0, Hash{}, Eq{}, AllocAdapter{std::move(Alloc)})
    {
    }

    /// @brief Build an empty HashMap with room for at least Cap elements.
    static HashMap withCapacity(SizeType Cap)
    {
        HashMap M;
        M.Inner.reserve(Cap);
        return M;
    }

    /// @brief Like withCapacity, but with an explicit allocator.
    static HashMap withCapacityIn(SizeType Cap, A Alloc)
    {
        HashMap M{std::move(Alloc)};
        M.Inner.reserve(Cap);
        return M;
    }

    HashMap(const HashMap&) = default;
    HashMap(HashMap&&) noexcept = default;
    HashMap& operator=(const HashMap&) = default;
    HashMap& operator=(HashMap&&) noexcept = default;
    ~HashMap() = default;

    /// @brief Number of key-value pairs.
    SizeType len() const noexcept { return Inner.size(); }

    /// @brief Whether the map has no entries.
    bool isEmpty() const noexcept { return Inner.empty(); }

    /// @brief Reserve space for at least Additional more entries.
    void reserve(SizeType Additional) { Inner.reserve(Inner.size() + Additional); }

    /// @brief Destroy all entries.
    void clear() noexcept { Inner.clear(); }

    /// @brief Insert or replace the mapping for Key. Returns the previous value if any.
    std::optional<V> insert(K Key, V Val)
    {
        auto It = Inner.find(Key);
        if (It == Inner.end())
        {
            Inner.emplace(std::move(Key), std::move(Val));
            return std::nullopt;
        }
        std::optional<V> Old{std::move(It->second)};
        It->second = std::move(Val);
        return Old;
    }

    /// @brief Remove the mapping for Key and return its value if present.
    std::optional<V> remove(const K& Key)
    {
        auto It = Inner.find(Key);
        if (It == Inner.end())
            return std::nullopt;
        std::optional<V> Out{std::move(It->second)};
        Inner.erase(It);
        return Out;
    }

    /// @brief Pointer to the value for Key, or nullptr if absent.
    const V* get(const K& Key) const noexcept
    {
        auto It = Inner.find(Key);
        return It == Inner.end() ? nullptr : std::addressof(It->second);
    }
    V* getMut(const K& Key) noexcept
    {
        auto It = Inner.find(Key);
        return It == Inner.end() ? nullptr : std::addressof(It->second);
    }

    /// @brief Whether the map contains an entry for Key.
    bool containsKey(const K& Key) const noexcept { return Inner.find(Key) != Inner.end(); }

    /// @brief Return an Entry handle for Key, allowing in-place insert / modify.
    /// @note  The returned Entry borrows *this. Do not insert or erase through the map while the
    ///        Entry is live; its cached iterator would be invalidated.
    Entry entry(K Key)
    {
        auto It = Inner.find(Key);
        return Entry{*this, std::move(Key), It};
    }

    /// @brief Reference to the value for Key, inserting Val if absent.
    V& entryOrInsert(K Key, V Val) { return entry(std::move(Key)).orInsert(std::move(Val)); }

    /// @brief Reference to the value for Key, inserting Make() if absent.
    template <class F>
    V& entryOrInsertWith(K Key, F Make)
    {
        return entry(std::move(Key)).orInsertWith(std::move(Make));
    }

    /// @brief Reference to the value for Key, inserting V{} if absent.
    V& entryOrDefault(K Key) { return entry(std::move(Key)).orDefault(); }

    /// @brief Keep only entries for which Predicate(key, value) returns true.
    template <class F>
    void retain(F Predicate)
    {
        for (auto It = Inner.begin(); It != Inner.end();)
        {
            if (!Predicate(It->first, It->second))
                It = Inner.erase(It);
            else
                ++It;
        }
    }

    Iterator begin() noexcept { return Inner.begin(); }
    Iterator end() noexcept { return Inner.end(); }
    ConstIterator begin() const noexcept { return Inner.begin(); }
    ConstIterator end() const noexcept { return Inner.end(); }
    ConstIterator cbegin() const noexcept { return Inner.cbegin(); }
    ConstIterator cend() const noexcept { return Inner.cend(); }

    /// @brief Element-wise equality.
    friend bool operator==(const HashMap& L, const HashMap& R) { return L.Inner == R.Inner; }

    /// @brief Const reference to the underlying allocator.
    const A& allocator() const noexcept { return Inner.get_allocator().Alloc; }

    /// @brief Chainable handle over a map slot. Returned by HashMap::entry(K).
    class Entry
    {
    private:
        friend class HashMap;

        HashMap* Owner;
        K Key_;
        Iterator It;

        Entry(HashMap& O, K KeyIn, Iterator ItIn) noexcept
            : Owner(&O)
            , Key_(std::move(KeyIn))
            , It(ItIn)
        {
        }

    public:
        Entry(const Entry&) = delete;
        Entry& operator=(const Entry&) = delete;
        Entry(Entry&&) noexcept = default;
        Entry& operator=(Entry&&) = delete;

        /// @brief Whether the map already contains an entry for this key.
        bool isOccupied() const noexcept { return It != Owner->Inner.end(); }

        /// @brief Whether the key is absent from the map.
        bool isVacant() const noexcept { return It == Owner->Inner.end(); }

        /// @brief The key being looked up (moves out of Entry if occupied==false; otherwise
        ///        returns a reference to the stored key).
        const K& key() const noexcept { return isOccupied() ? It->first : Key_; }

        /// @brief Reference to the existing value, or insert Val and return its reference.
        V& orInsert(V Val)
        {
            if (isVacant())
            {
                It = Owner->Inner.emplace(std::move(Key_), std::move(Val)).first;
            }
            return It->second;
        }

        /// @brief Like orInsert, but constructs the default lazily via Make().
        template <class F>
        V& orInsertWith(F Make)
        {
            if (isVacant())
            {
                It = Owner->Inner.emplace(std::move(Key_), Make()).first;
            }
            return It->second;
        }

        /// @brief Like orInsert, but the default is a value-initialised V.
        V& orDefault()
        {
            if (isVacant())
            {
                It = Owner->Inner.emplace(std::move(Key_), V{}).first;
            }
            return It->second;
        }

        /// @brief Apply Mutator to the existing value (no-op if vacant) and return *this.
        template <class F>
        Entry&& andModify(F Mutator) &&
        {
            if (isOccupied())
                Mutator(It->second);
            return std::move(*this);
        }
        template <class F>
        Entry& andModify(F Mutator) &
        {
            if (isOccupied())
                Mutator(It->second);
            return *this;
        }
    };
};

} // namespace goleta
