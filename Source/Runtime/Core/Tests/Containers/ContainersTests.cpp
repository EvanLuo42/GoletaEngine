/// @file
/// @brief Unit tests for goleta container classes and allocator plumbing.

#include "Core.h"

#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <utility>

using namespace goleta;

TEST(VecTest, DefaultConstructIsEmpty)
{
    Vec<int> V;
    EXPECT_EQ(V.len(), 0u);
    EXPECT_TRUE(V.isEmpty());
    EXPECT_EQ(V.first(), nullptr);
    EXPECT_EQ(V.last(), nullptr);
    EXPECT_EQ(V.get(0), nullptr);
}

TEST(VecTest, ConstructWithCount)
{
    Vec<int> V(5);
    EXPECT_EQ(V.len(), 5u);
    for (int X : V) EXPECT_EQ(X, 0);
}

TEST(VecTest, ConstructFillValue)
{
    Vec<int> V(3, 7);
    EXPECT_EQ(V.len(), 3u);
    EXPECT_EQ(V[0], 7);
    EXPECT_EQ(V[1], 7);
    EXPECT_EQ(V[2], 7);
}

TEST(VecTest, ConstructFromIteratorRange)
{
    int Data[] = {1, 2, 3, 4};
    Vec<int> V(std::begin(Data), std::end(Data));
    EXPECT_EQ(V.len(), 4u);
    EXPECT_EQ(V[3], 4);
}

TEST(VecTest, ConstructFromInitializerList)
{
    Vec<int> V{10, 20, 30};
    EXPECT_EQ(V.len(), 3u);
    EXPECT_EQ(V[1], 20);
}

TEST(VecTest, WithCapacityAllocatesButEmpty)
{
    auto V = Vec<int>::withCapacity(64);
    EXPECT_TRUE(V.isEmpty());
    EXPECT_GE(V.capacity(), 64u);
}

TEST(VecTest, PushCopyAndMove)
{
    Vec<std::string> V;
    std::string A = "alpha";
    V.push(A);
    V.push(std::string("beta"));
    EXPECT_EQ(V.len(), 2u);
    EXPECT_EQ(V[0], "alpha");
    EXPECT_EQ(V[1], "beta");
}

TEST(VecTest, EmplaceReturnsReference)
{
    Vec<std::pair<int, int>> V;
    auto& Ref = V.emplace(1, 2);
    Ref.second = 42;
    EXPECT_EQ(V[0].second, 42);
}

TEST(VecTest, PopReturnsOptionalAndEmptiesOnNone)
{
    Vec<int> V{1, 2, 3};
    auto X = V.pop();
    ASSERT_TRUE(X.has_value());
    EXPECT_EQ(*X, 3);
    EXPECT_EQ(V.len(), 2u);

    Vec<int> Empty;
    EXPECT_FALSE(Empty.pop().has_value());
}

TEST(VecTest, InsertAndRemove)
{
    Vec<int> V{1, 2, 4};
    V.insert(2, 3);
    EXPECT_EQ(V.len(), 4u);
    EXPECT_EQ(V[2], 3);

    int Removed = V.remove(0);
    EXPECT_EQ(Removed, 1);
    EXPECT_EQ(V[0], 2);
}

TEST(VecTest, SwapRemoveReordersFromEnd)
{
    Vec<int> V{10, 20, 30, 40};
    int Out = V.swapRemove(1);
    EXPECT_EQ(Out, 20);
    EXPECT_EQ(V.len(), 3u);
    EXPECT_EQ(V[1], 40);
}

TEST(VecTest, ClearAndTruncate)
{
    Vec<int> V{1, 2, 3, 4, 5};
    V.truncate(3);
    EXPECT_EQ(V.len(), 3u);
    EXPECT_EQ(V[2], 3);
    V.truncate(10);
    EXPECT_EQ(V.len(), 3u);
    V.clear();
    EXPECT_TRUE(V.isEmpty());
}

TEST(VecTest, ResizeAndResizeWith)
{
    Vec<int> V;
    V.resize(3, 9);
    EXPECT_EQ(V[0], 9);
    EXPECT_EQ(V.len(), 3u);

    V.resize(1);
    EXPECT_EQ(V.len(), 1u);

    int Counter = 0;
    V.resizeWith(4, [&]{ return ++Counter; });
    EXPECT_EQ(V.len(), 4u);
    EXPECT_EQ(V[1], 1);
    EXPECT_EQ(V[3], 3);
}

TEST(VecTest, FillAndFillWith)
{
    Vec<int> V(3, 0);
    V.fill(5);
    for (int X : V) EXPECT_EQ(X, 5);

    int Counter = 0;
    V.fillWith([&]{ return ++Counter; });
    EXPECT_EQ(V[0], 1);
    EXPECT_EQ(V[2], 3);
}

TEST(VecTest, AppendMovesFromOther)
{
    Vec<int> A{1, 2};
    Vec<int> B{3, 4, 5};
    A.append(B);
    EXPECT_EQ(A.len(), 5u);
    EXPECT_EQ(A[4], 5);
    EXPECT_TRUE(B.isEmpty());
}

TEST(VecTest, ExtendFromSliceAndIteratorRange)
{
    Vec<int> V{1};
    int More[] = {2, 3};
    V.extendFromSlice(std::span<const int>(More, 2));
    EXPECT_EQ(V.len(), 3u);

    int Even[] = {4, 5};
    V.extend(std::begin(Even), std::end(Even));
    EXPECT_EQ(V.len(), 5u);
    EXPECT_EQ(V[4], 5);
}

TEST(VecTest, RetainKeepsPredicateMatches)
{
    Vec<int> V{1, 2, 3, 4, 5, 6};
    V.retain([](int X){ return X % 2 == 0; });
    EXPECT_EQ(V.len(), 3u);
    EXPECT_EQ(V[0], 2);
    EXPECT_EQ(V[2], 6);
}

TEST(VecTest, ContainsAndPosition)
{
    Vec<int> V{10, 20, 30};
    EXPECT_TRUE(V.contains(20));
    EXPECT_FALSE(V.contains(99));
    EXPECT_EQ(*V.position(30), 2u);
    EXPECT_FALSE(V.position(99).has_value());
}

TEST(VecTest, SortAndSortByAndDedup)
{
    Vec<int> V{3, 1, 4, 1, 5, 9, 2, 6};
    V.sort();
    EXPECT_EQ(V[0], 1);
    EXPECT_EQ(V[V.len() - 1], 9);

    V.dedup();
    EXPECT_EQ(V.len(), 7u);

    V.sortBy([](int A, int B){ return A > B; });
    EXPECT_EQ(V[0], 9);

    Vec<int> U{5, 3, 8};
    U.sortUnstable();
    EXPECT_EQ(U[0], 3);
    EXPECT_EQ(U[2], 8);
}

TEST(VecTest, SliceAccessors)
{
    Vec<int> V{1, 2, 3};
    std::span<const int> S = V.asSlice();
    EXPECT_EQ(S.size(), 3u);
    EXPECT_EQ(S[0], 1);

    std::span<int> M = V.asMutSlice();
    M[0] = 99;
    EXPECT_EQ(V[0], 99);

    EXPECT_EQ(V.data(), V.asSlice().data());
}

TEST(VecTest, FirstAndLastGetAndGetMut)
{
    Vec<int> V{7, 8, 9};
    EXPECT_EQ(*V.first(), 7);
    EXPECT_EQ(*V.last(), 9);
    *V.getMut(1) = 42;
    EXPECT_EQ(*V.get(1), 42);
    EXPECT_EQ(V.get(99), nullptr);
}

TEST(VecTest, ReverseIteration)
{
    Vec<int> V{1, 2, 3};
    auto It = V.rbegin();
    EXPECT_EQ(*It++, 3);
    EXPECT_EQ(*It++, 2);
    EXPECT_EQ(*It, 1);
    (void)V.rend();
}

TEST(VecTest, IterAndIterMutViews)
{
    Vec<int> V{1, 2, 3};
    int Sum = 0;
    for (int X : V.iter()) Sum += X;
    EXPECT_EQ(Sum, 6);

    for (int& X : V.iterMut()) X *= 10;
    EXPECT_EQ(V[2], 30);
}

TEST(VecTest, CapacityOps)
{
    Vec<int> V;
    V.reserve(32);
    EXPECT_GE(V.capacity(), 32u);

    const auto CapAfterReserve = V.capacity();
    V.reserveExact(16);
    // 'Additional' 16 is already satisfied by existing capacity; reserveExact is a no-op.
    EXPECT_EQ(V.capacity(), CapAfterReserve);

    V.reserveExact(V.capacity() + 1);
    EXPECT_GE(V.capacity(), CapAfterReserve + 1);

    V.push(1);
    V.shrinkToFit();
    EXPECT_GE(V.capacity(), 1u);

    auto W = Vec<int>::withCapacity(100);
    W.push(1);
    W.push(2);
    W.shrinkTo(8);
    EXPECT_LE(W.capacity(), 100u);
    EXPECT_GE(W.capacity(), W.len());
}

TEST(VecTest, CopyAndMoveAndEquality)
{
    Vec<int> A{1, 2, 3};
    Vec<int> B = A;
    EXPECT_EQ(A, B);

    Vec<int> C = std::move(A);
    EXPECT_EQ(C.len(), 3u);
    EXPECT_TRUE(A.isEmpty() || !A.isEmpty());

    Vec<int> D{1, 2, 4};
    EXPECT_FALSE(B == D);
    EXPECT_TRUE(B < D);
}

TEST(VecTest, IndexOutOfBoundsReturnsNullForGet)
{
    Vec<int> V{1};
    EXPECT_NE(V.get(0), nullptr);
    EXPECT_EQ(V.get(1), nullptr);
}

TEST(VecTest, AllocatorAccessor)
{
    Vec<int> V;
    (void)V.allocator();
    SUCCEED();
}

TEST(HashMapTest, InsertReturnsPreviousValue)
{
    HashMap<std::string, int> M;
    EXPECT_FALSE(M.insert("k", 1).has_value());
    auto Old = M.insert("k", 2);
    ASSERT_TRUE(Old.has_value());
    EXPECT_EQ(*Old, 1);
    EXPECT_EQ(M.len(), 1u);
}

TEST(HashMapTest, GetAndGetMutAndContainsKey)
{
    HashMap<std::string, int> M;
    M.insert("k", 10);
    EXPECT_EQ(*M.get("k"), 10);
    EXPECT_EQ(M.get("missing"), nullptr);
    EXPECT_TRUE(M.containsKey("k"));
    EXPECT_FALSE(M.containsKey("missing"));

    *M.getMut("k") = 99;
    EXPECT_EQ(*M.get("k"), 99);
    EXPECT_EQ(M.getMut("missing"), nullptr);
}

TEST(HashMapTest, RemoveReturnsOldValue)
{
    HashMap<int, std::string> M;
    M.insert(1, "one");
    M.insert(2, "two");
    auto Out = M.remove(1);
    ASSERT_TRUE(Out.has_value());
    EXPECT_EQ(*Out, "one");
    EXPECT_EQ(M.len(), 1u);
    EXPECT_FALSE(M.remove(99).has_value());
}

TEST(HashMapTest, EntryOrInsertAndWithAndDefault)
{
    HashMap<std::string, int> M;
    M.entryOrInsert("a", 1);
    M.entryOrInsert("a", 99);
    EXPECT_EQ(*M.get("a"), 1);

    int& B = M.entryOrInsertWith("b", []{ return 7; });
    EXPECT_EQ(B, 7);
    B = 100;
    EXPECT_EQ(*M.get("b"), 100);

    int& C = M.entryOrDefault("c");
    EXPECT_EQ(C, 0);
}

TEST(HashMapTest, RetainAndClearAndIsEmpty)
{
    HashMap<int, int> M;
    for (int I = 0; I < 6; ++I) M.insert(I, I * 10);
    M.retain([](int K, int&){ return K % 2 == 0; });
    EXPECT_EQ(M.len(), 3u);
    EXPECT_TRUE(M.containsKey(0));
    EXPECT_FALSE(M.containsKey(1));

    M.clear();
    EXPECT_TRUE(M.isEmpty());
}

TEST(HashMapTest, WithCapacityAndReserve)
{
    auto M = HashMap<int, int>::withCapacity(64);
    EXPECT_TRUE(M.isEmpty());
    M.reserve(32);
    SUCCEED();
}

TEST(HashMapTest, IterationAndEquality)
{
    HashMap<int, int> A;
    A.insert(1, 10);
    A.insert(2, 20);

    int Sum = 0;
    for (const auto& [K, V] : A) Sum += V;
    EXPECT_EQ(Sum, 30);

    HashMap<int, int> B;
    B.insert(2, 20);
    B.insert(1, 10);
    EXPECT_EQ(A, B);
}

TEST(HashMapTest, AllocatorAccessor)
{
    HashMap<int, int> M;
    (void)M.allocator();
    SUCCEED();
}

TEST(BoxTest, DefaultIsNull)
{
    Box<int> B;
    EXPECT_FALSE(B);
    EXPECT_TRUE(B.isNull());
    EXPECT_EQ(B.get(), nullptr);
    EXPECT_EQ(B, nullptr);
}

TEST(BoxTest, MakeBoxAndDeref)
{
    Box<int> B = makeBox<int>(42);
    EXPECT_TRUE(B);
    EXPECT_EQ(*B, 42);
    EXPECT_EQ(B.asRef(), 42);
    B.asMut() = 99;
    EXPECT_EQ(*B, 99);
}

TEST(BoxTest, MoveTransfersOwnership)
{
    Box<int> A = makeBox<int>(7);
    Box<int> B = std::move(A);
    EXPECT_TRUE(A.isNull());
    ASSERT_TRUE(B);
    EXPECT_EQ(*B, 7);
}

TEST(BoxTest, ReleaseAndReset)
{
    Box<int> B = makeBox<int>(5);
    int* Raw = B.release();
    EXPECT_TRUE(B.isNull());
    ASSERT_NE(Raw, nullptr);

    B.reset(Raw);
    EXPECT_TRUE(B);
    EXPECT_EQ(*B, 5);

    B.reset();
    EXPECT_TRUE(B.isNull());
}

TEST(BoxTest, ArrowOperator)
{
    struct Point { int X = 1; int sum() const { return X; } };
    Box<Point> P = makeBox<Point>();
    EXPECT_EQ(P->sum(), 1);
    P->X = 10;
    EXPECT_EQ(P->X, 10);
}

TEST(BoxTest, UpcastOnMoveConstruction)
{
    struct Base { virtual ~Base() = default; virtual int tag() const { return 1; } };
    struct Derived : Base { int tag() const override { return 2; } };

    Box<Derived> D = makeBox<Derived>();
    Box<Base> B = std::move(D);
    ASSERT_TRUE(B);
    EXPECT_EQ(B->tag(), 2);
}

TEST(BoxTest, SwapExchanges)
{
    Box<int> A = makeBox<int>(1);
    Box<int> B = makeBox<int>(2);
    A.swap(B);
    EXPECT_EQ(*A, 2);
    EXPECT_EQ(*B, 1);
}

TEST(BoxTest, ExplicitRawConstruction)
{
    Box<int> B{new int(17)};
    EXPECT_TRUE(B);
    EXPECT_EQ(*B, 17);
}

TEST(ArcTest, DefaultIsNull)
{
    Arc<int> A;
    EXPECT_FALSE(A);
    EXPECT_TRUE(A.isNull());
    EXPECT_EQ(A.get(), nullptr);
    EXPECT_EQ(A.strongCount(), 0);
    EXPECT_EQ(A, nullptr);
}

TEST(ArcTest, MakeArcAndDeref)
{
    Arc<int> A = makeArc<int>(7);
    EXPECT_TRUE(A);
    EXPECT_EQ(*A, 7);
    EXPECT_EQ(A.strongCount(), 1);
}

TEST(ArcTest, CopyIncrementsStrongCount)
{
    Arc<int> A = makeArc<int>(1);
    {
        Arc<int> B = A;
        EXPECT_EQ(A.strongCount(), 2);
        EXPECT_EQ(B.strongCount(), 2);
        EXPECT_EQ(A, B);
    }
    EXPECT_EQ(A.strongCount(), 1);
}

TEST(ArcTest, MoveDoesNotIncrementCount)
{
    Arc<int> A = makeArc<int>(5);
    Arc<int> B = std::move(A);
    EXPECT_EQ(B.strongCount(), 1);
    EXPECT_TRUE(A.isNull());
}

TEST(ArcTest, DowngradeAndReset)
{
    Arc<int> A = makeArc<int>(9);
    Weak<int> W = A.downgrade();
    EXPECT_FALSE(W.isExpired());

    A.reset();
    EXPECT_TRUE(A.isNull());
    EXPECT_TRUE(W.isExpired());
}

TEST(ArcTest, UpcastCopyFromDerived)
{
    struct Base { virtual ~Base() = default; virtual int tag() const { return 1; } };
    struct Derived : Base { int tag() const override { return 2; } };

    Arc<Derived> D = makeArc<Derived>();
    Arc<Base> B = D;
    EXPECT_EQ(D.strongCount(), 2);
    EXPECT_EQ(B->tag(), 2);
}

TEST(ArcTest, SwapAndAsSharedPtr)
{
    Arc<int> A = makeArc<int>(1);
    Arc<int> B = makeArc<int>(2);
    A.swap(B);
    EXPECT_EQ(*A, 2);
    EXPECT_EQ(*B, 1);

    const std::shared_ptr<int>& S = A.asSharedPtr();
    EXPECT_EQ(*S, 2);
}

TEST(ArcTest, ExplicitRawConstruction)
{
    Arc<int> A{new int(33)};
    EXPECT_EQ(*A, 33);
    EXPECT_EQ(A.strongCount(), 1);
}

TEST(WeakTest, DefaultIsExpired)
{
    Weak<int> W;
    EXPECT_TRUE(W.isExpired());
    EXPECT_EQ(W.strongCount(), 0);
    EXPECT_TRUE(W.upgrade().isNull());
}

TEST(WeakTest, UpgradeReturnsLiveArc)
{
    Arc<int> A = makeArc<int>(4);
    Weak<int> W = A.downgrade();
    Arc<int> B = W.upgrade();
    ASSERT_FALSE(B.isNull());
    EXPECT_EQ(*B, 4);
    EXPECT_EQ(A.strongCount(), 2);
}

TEST(WeakTest, UpgradeAfterExpiryReturnsNull)
{
    Weak<int> W;
    {
        Arc<int> A = makeArc<int>(1);
        W = A.downgrade();
    }
    EXPECT_TRUE(W.isExpired());
    EXPECT_TRUE(W.upgrade().isNull());
}

TEST(WeakTest, ResetAndSwap)
{
    Arc<int> A = makeArc<int>(1);
    Arc<int> B = makeArc<int>(2);
    Weak<int> WA = A.downgrade();
    Weak<int> WB = B.downgrade();
    WA.swap(WB);
    EXPECT_EQ(*WA.upgrade(), 2);
    EXPECT_EQ(*WB.upgrade(), 1);

    WA.reset();
    EXPECT_TRUE(WA.isExpired());
}

namespace {

struct CountingAllocatorState
{
    std::size_t BytesAllocated = 0;
    std::size_t BytesFreed     = 0;
};

struct CountingAllocator
{
    CountingAllocatorState* State = nullptr;

    CountingAllocator() = default;
    explicit CountingAllocator(CountingAllocatorState& S) : State(&S) {}

    template <class T>
    [[nodiscard]] T* allocate(std::size_t N) const
    {
        if (State) State->BytesAllocated += N * sizeof(T);
        return static_cast<T*>(::operator new(N * sizeof(T), std::align_val_t{alignof(T)}));
    }
    template <class T>
    void deallocate(T* P, std::size_t N) const noexcept
    {
        if (State) State->BytesFreed += N * sizeof(T);
        ::operator delete(static_cast<void*>(P), N * sizeof(T), std::align_val_t{alignof(T)});
    }
    bool operator==(const CountingAllocator& O) const noexcept { return State == O.State; }
    bool operator!=(const CountingAllocator& O) const noexcept { return !(*this == O); }
};

struct EmptyTraitAllocator
{
    template <class T> T* allocate(std::size_t) const { return nullptr; }
    template <class T> void deallocate(T*, std::size_t) const noexcept {}
    bool operator==(const EmptyTraitAllocator&) const noexcept { return true; }
};

struct StatefulOptOutAllocator
{
    int* State = nullptr;
    using PropagateOnCopy = std::false_type;
    using PropagateOnMove = std::false_type;
    using PropagateOnSwap = std::false_type;
    using IsAlwaysEqual   = std::false_type;
    template <class T> T* allocate(std::size_t) const { return nullptr; }
    template <class T> void deallocate(T*, std::size_t) const noexcept {}
    bool operator==(const StatefulOptOutAllocator& O) const noexcept { return State == O.State; }
};

} // namespace

TEST(AllocatorTest, GlobalIsEmptyAlwaysEqual)
{
    EXPECT_TRUE(std::is_empty_v<allocators::Global>);
    EXPECT_TRUE(allocators::Global{} == allocators::Global{});
}

TEST(AllocatorTest, VecRoutesThroughCustomAllocator)
{
    CountingAllocatorState State;
    {
        Vec<int, CountingAllocator> V{CountingAllocator{State}};
        for (int I = 0; I < 64; ++I) V.push(I);
        EXPECT_GT(State.BytesAllocated, 0u);
    }
    // Every byte the allocator handed out has been returned by the time the Vec is destroyed.
    EXPECT_EQ(State.BytesAllocated, State.BytesFreed);
}

TEST(AllocatorTest, HashMapRebindsToPairNode)
{
    CountingAllocatorState State;
    HashMap<int, int, std::hash<int>, std::equal_to<int>, CountingAllocator> M{
        CountingAllocator{State}};
    for (int I = 0; I < 32; ++I) M.insert(I, I * 2);
    EXPECT_GT(State.BytesAllocated, 0u);
}

TEST(BoxTest, MakeBoxInRoutesThroughAllocator)
{
    CountingAllocatorState State;
    {
        Box<int, CountingAllocator> B = makeBoxIn<int>(CountingAllocator{State}, 42);
        ASSERT_TRUE(B);
        EXPECT_EQ(*B, 42);
        EXPECT_GT(State.BytesAllocated, 0u);
    }
    EXPECT_EQ(State.BytesAllocated, State.BytesFreed);
}

TEST(BoxTest, LeakReleasesWithoutFree)
{
    Box<int> B = makeBox<int>(5);
    int* Raw = B.leak();
    EXPECT_TRUE(B.isNull());
    ASSERT_NE(Raw, nullptr);
    EXPECT_EQ(*Raw, 5);
    // Ownership is now ours; return it to the runtime manually.
    allocators::Global{}.deallocate(Raw, 1);
}

TEST(BoxTest, AllocatorAccessor)
{
    Box<int> B = makeBox<int>(1);
    (void)B.allocator();
    SUCCEED();
}

TEST(ArcTest, MakeArcInRoutesThroughAllocator)
{
    CountingAllocatorState State;
    {
        Arc<int, CountingAllocator> A = makeArcIn<int>(CountingAllocator{State}, 9);
        ASSERT_TRUE(A);
        EXPECT_EQ(*A, 9);
        EXPECT_EQ(A.strongCount(), 1);
        EXPECT_GT(State.BytesAllocated, 0u);
    }
    EXPECT_EQ(State.BytesAllocated, State.BytesFreed);
}

TEST(HashMapEntryTest, VacantOrInsertInsertsAndReturnsReference)
{
    HashMap<int, int> M;
    int& V = M.entry(3).orInsert(30);
    EXPECT_EQ(V, 30);
    EXPECT_EQ(M.len(), 1u);
    V = 31;
    EXPECT_EQ(*M.get(3), 31);
}

TEST(HashMapEntryTest, OccupiedOrInsertDoesNotOverwrite)
{
    HashMap<int, int> M;
    M.insert(1, 100);
    int& V = M.entry(1).orInsert(999);
    EXPECT_EQ(V, 100);
}

TEST(HashMapEntryTest, OrInsertWithLazyAndOrDefault)
{
    HashMap<std::string, int> M;
    int Calls = 0;
    int& V = M.entry("k").orInsertWith([&]{ ++Calls; return 7; });
    EXPECT_EQ(V, 7);
    EXPECT_EQ(Calls, 1);

    (void)M.entry("k").orInsertWith([&]{ ++Calls; return 0; });
    EXPECT_EQ(Calls, 1);

    int& D = M.entry("fresh").orDefault();
    EXPECT_EQ(D, 0);
}

TEST(HashMapEntryTest, AndModifyChainable)
{
    HashMap<int, int> M;
    M.insert(1, 10);
    M.entry(1).andModify([](int& V){ V += 5; }).orInsert(0);
    EXPECT_EQ(*M.get(1), 15);

    M.entry(2).andModify([](int& V){ V += 5; }).orInsert(100);
    EXPECT_EQ(*M.get(2), 100);
}

TEST(HashMapEntryTest, OccupiedQueries)
{
    HashMap<int, int> M;
    M.insert(1, 10);
    auto E = M.entry(1);
    EXPECT_TRUE(E.isOccupied());
    EXPECT_FALSE(E.isVacant());
    EXPECT_EQ(E.key(), 1);

    auto V = M.entry(2);
    EXPECT_TRUE(V.isVacant());
    EXPECT_EQ(V.key(), 2);
}

TEST(StringTest, DefaultIsEmpty)
{
    String S;
    EXPECT_EQ(S.len(), 0u);
    EXPECT_TRUE(S.isEmpty());
    EXPECT_STREQ(S.cStr(), "");
}

TEST(StringTest, ConstructFromCStringAndStringView)
{
    String A = "hello";
    EXPECT_EQ(A.len(), 5u);
    EXPECT_STREQ(A.cStr(), "hello");

    StringView V = "world";
    String B(V);
    EXPECT_EQ(B.len(), 5u);
    EXPECT_TRUE(B == V);
}

TEST(StringTest, RepeatedChar)
{
    String S(3, 'x');
    EXPECT_EQ(S, "xxx");
}

TEST(StringTest, PushAndPushStr)
{
    String S;
    S.push('h');
    S.pushStr("ey");
    EXPECT_EQ(S, "hey");

    S += '!';
    S += StringView{"!"};
    EXPECT_EQ(S, "hey!!");
}

TEST(StringTest, PopAndRemoveAndInsert)
{
    String S = "abcd";
    auto C = S.pop();
    ASSERT_TRUE(C.has_value());
    EXPECT_EQ(*C, 'd');
    EXPECT_EQ(S, "abc");

    char R = S.remove(1);
    EXPECT_EQ(R, 'b');
    EXPECT_EQ(S, "ac");

    S.insert(1, 'Z');
    EXPECT_EQ(S, "aZc");
    S.insertStr(0, "[");
    EXPECT_EQ(S, "[aZc");
}

TEST(StringTest, TruncateClear)
{
    String S = "abcdef";
    S.truncate(3);
    EXPECT_EQ(S, "abc");
    S.truncate(10);
    EXPECT_EQ(S, "abc");
    S.clear();
    EXPECT_TRUE(S.isEmpty());
}

TEST(StringTest, FindContainsStartsWithEndsWith)
{
    String S = "fast and fast-ish";
    EXPECT_EQ(S.find("fast"), 0u);
    EXPECT_EQ(S.find('a'), 1u);
    EXPECT_EQ(S.find("zzz"), String::Npos);
    EXPECT_TRUE(S.contains("and"));
    EXPECT_FALSE(S.contains("zzz"));
    EXPECT_TRUE(S.startsWith("fast"));
    EXPECT_FALSE(S.startsWith("slow"));
    EXPECT_TRUE(S.endsWith("ish"));
    EXPECT_FALSE(S.endsWith("ish!"));
}

TEST(StringTest, PopOnEmpty)
{
    String S;
    EXPECT_FALSE(S.pop().has_value());
}

TEST(StringTest, CapacityOps)
{
    auto S = String::withCapacity(64);
    EXPECT_TRUE(S.isEmpty());
    EXPECT_GE(S.capacity(), 64u);
    S.reserve(1000);
    EXPECT_GE(S.capacity(), 1000u);
    S.push('x');
    S.shrinkToFit();
    EXPECT_GE(S.capacity(), 1u);
}

TEST(StringTest, AsStrAndAsBytesAndImplicitView)
{
    String S = "abc";
    StringView V = S.asStr();
    EXPECT_EQ(V.size(), 3u);
    EXPECT_EQ(V[1], 'b');

    auto Bytes = S.asBytes();
    EXPECT_EQ(Bytes.size(), 3u);
    EXPECT_EQ(Bytes[0], static_cast<std::uint8_t>('a'));

    StringView Implicit = S;
    EXPECT_EQ(Implicit, "abc");
}

TEST(StringTest, IterationAndIndex)
{
    String S = "abc";
    int Sum = 0;
    for (char C : S) Sum += C;
    EXPECT_EQ(Sum, 'a' + 'b' + 'c');
    EXPECT_EQ(S[0], 'a');
    S[0] = 'X';
    EXPECT_EQ(S, "Xbc");
}

TEST(StringTest, OperatorPlusConcatenation)
{
    String A = "foo";
    String B = A + StringView{"bar"};
    EXPECT_EQ(B, "foobar");
    String C = StringView{"pre-"} + A;
    EXPECT_EQ(C, "pre-foo");
}

TEST(StringTest, ComparisonsCrossType)
{
    String A = "abc";
    EXPECT_TRUE(A == "abc");
    EXPECT_TRUE("abc" == A);
    EXPECT_TRUE(A == StringView{"abc"});
    EXPECT_FALSE(A == "abd");

    String B = "abd";
    EXPECT_TRUE(A < B);
}

TEST(StringTest, InsertAtEndEquivalentToPush)
{
    String S = "abc";
    S.insert(S.len(), 'd');
    EXPECT_EQ(S, "abcd");
    S.insertStr(S.len(), "ef");
    EXPECT_EQ(S, "abcdef");
}

TEST(StringTest, WithCapacityInCustomAllocator)
{
    CountingAllocatorState State;
    {
        BasicString<CountingAllocator> S =
            BasicString<CountingAllocator>::withCapacityIn(32, CountingAllocator{State});
        S.pushStr("hello, allocator");
        EXPECT_EQ(S.len(), 16u);
    }
    EXPECT_EQ(State.BytesAllocated, State.BytesFreed);
    EXPECT_GT(State.BytesAllocated, 0u);
}

TEST(AllocatorTraitsTest, EmptyAllocatorGetsAlwaysEqualByDefault)
{
    using Adapter = detail::StdAllocatorAdapter<int, EmptyTraitAllocator>;
    EXPECT_TRUE(Adapter::is_always_equal::value);
    EXPECT_TRUE(Adapter::propagate_on_container_copy_assignment::value);
}

TEST(AllocatorTraitsTest, StatefulAllocatorCanOptOutOfPropagation)
{
    using Adapter = detail::StdAllocatorAdapter<int, StatefulOptOutAllocator>;
    EXPECT_FALSE(Adapter::is_always_equal::value);
    EXPECT_FALSE(Adapter::propagate_on_container_copy_assignment::value);
    EXPECT_FALSE(Adapter::propagate_on_container_move_assignment::value);
    EXPECT_FALSE(Adapter::propagate_on_container_swap::value);
}
