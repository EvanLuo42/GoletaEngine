/// @file
/// @brief Unit tests for Rc<T> and RefCounted.

#include <gtest/gtest.h>

#include <utility>

#include "Memory/Rc.h"

namespace
{

using namespace goleta;

class Tracer : public RefCounted
{
public:
    explicit Tracer(int* DtorCounter)
        : Dtor(DtorCounter)
    {
    }
    int Value = 0;

protected:
    ~Tracer() override
    {
        if (Dtor)
            ++*Dtor;
    }

private:
    int* Dtor;
};

class DerivedTracer : public Tracer
{
public:
    using Tracer::Tracer;
    int Extra = 42;

protected:
    ~DerivedTracer() override = default;
};

} // namespace

TEST(RcTest, DefaultConstructedIsNull)
{
    Rc<Tracer> P;
    EXPECT_TRUE(P.isNull());
    EXPECT_FALSE(static_cast<bool>(P));
}

TEST(RcTest, NullptrConstructedIsNull)
{
    Rc<Tracer> P(nullptr);
    EXPECT_TRUE(P.isNull());
}

TEST(RcTest, MakeRcProducesRefCountOne)
{
    int Dtors = 0;
    {
        auto P = makeRc<Tracer>(&Dtors);
        ASSERT_TRUE(static_cast<bool>(P));
        EXPECT_EQ(P->refCount(), 1);
    }
    EXPECT_EQ(Dtors, 1);
}

TEST(RcTest, CopyBumpsRefCount)
{
    int Dtors = 0;
    {
        auto A = makeRc<Tracer>(&Dtors);
        {
            Rc<Tracer> B = A;
            EXPECT_EQ(A->refCount(), 2);
            EXPECT_EQ(A.get(), B.get());
        }
        EXPECT_EQ(A->refCount(), 1);
        EXPECT_EQ(Dtors, 0);
    }
    EXPECT_EQ(Dtors, 1);
}

TEST(RcTest, MoveTransfersWithoutBumping)
{
    int Dtors = 0;
    auto A = makeRc<Tracer>(&Dtors);
    Tracer* const Raw = A.get();
    Rc<Tracer> B = std::move(A);
    EXPECT_TRUE(A.isNull());
    EXPECT_EQ(B.get(), Raw);
    EXPECT_EQ(B->refCount(), 1);
}

TEST(RcTest, AssignmentOverwritesAndDecrements)
{
    int DtorsA = 0;
    int DtorsB = 0;
    auto A = makeRc<Tracer>(&DtorsA);
    auto B = makeRc<Tracer>(&DtorsB);
    A = B;
    EXPECT_EQ(DtorsA, 1);
    EXPECT_EQ(DtorsB, 0);
    EXPECT_EQ(A.get(), B.get());
    EXPECT_EQ(A->refCount(), 2);
}

TEST(RcTest, MoveAssignmentReleasesPrevious)
{
    int DtorsA = 0;
    int DtorsB = 0;
    auto A = makeRc<Tracer>(&DtorsA);
    auto B = makeRc<Tracer>(&DtorsB);
    A = std::move(B);
    EXPECT_EQ(DtorsA, 1);
    EXPECT_TRUE(B.isNull());
    EXPECT_EQ(A->refCount(), 1);
}

TEST(RcTest, ResetReleasesPreviousAndAdoptsNew)
{
    int Dtors1 = 0;
    int Dtors2 = 0;
    auto P = makeRc<Tracer>(&Dtors1);
    Tracer* const NewPtr = new Tracer(&Dtors2);
    P.reset(NewPtr);
    EXPECT_EQ(Dtors1, 1);
    EXPECT_EQ(Dtors2, 0);
    EXPECT_EQ(P.get(), NewPtr);
    EXPECT_EQ(P->refCount(), 1);
}

TEST(RcTest, ResetToNullReleasesPrevious)
{
    int Dtors = 0;
    auto P = makeRc<Tracer>(&Dtors);
    P.reset();
    EXPECT_EQ(Dtors, 1);
    EXPECT_TRUE(P.isNull());
}

TEST(RcTest, UpcastCopyShareOwnership)
{
    int Dtors = 0;
    auto D = makeRc<DerivedTracer>(&Dtors);
    Rc<Tracer> B = D;
    EXPECT_EQ(B.get(), D.get());
    EXPECT_EQ(D->refCount(), 2);
}

TEST(RcTest, UpcastMoveTransfersOwnership)
{
    int Dtors = 0;
    auto D = makeRc<DerivedTracer>(&Dtors);
    Tracer* const Raw = D.get();
    Rc<Tracer> B = std::move(D);
    EXPECT_TRUE(D.isNull());
    EXPECT_EQ(B.get(), Raw);
    EXPECT_EQ(B->refCount(), 1);
}

TEST(RcTest, LeakTransfersOwnership)
{
    int Dtors = 0;
    auto P = makeRc<Tracer>(&Dtors);
    Tracer* const Raw = P.leak();
    EXPECT_TRUE(P.isNull());
    EXPECT_EQ(Raw->refCount(), 1);
    Raw->release();
    EXPECT_EQ(Dtors, 1);
}

TEST(RcTest, SwapExchangesPointers)
{
    int DtorsA = 0;
    int DtorsB = 0;
    auto A = makeRc<Tracer>(&DtorsA);
    auto B = makeRc<Tracer>(&DtorsB);
    Tracer* const RawA = A.get();
    Tracer* const RawB = B.get();
    A.swap(B);
    EXPECT_EQ(A.get(), RawB);
    EXPECT_EQ(B.get(), RawA);
    EXPECT_EQ(DtorsA, 0);
    EXPECT_EQ(DtorsB, 0);
}

TEST(RcTest, NullptrComparison)
{
    Rc<Tracer> P;
    EXPECT_EQ(P, nullptr);
    EXPECT_EQ(nullptr, P);

    int Dtors = 0;
    auto Q = makeRc<Tracer>(&Dtors);
    EXPECT_FALSE(Q == nullptr);
}

TEST(RcTest, EqualPointerCompareEqual)
{
    int Dtors = 0;
    auto A = makeRc<Tracer>(&Dtors);
    Rc<Tracer> B = A;
    EXPECT_EQ(A, B);
}

TEST(RefCountedTest, NewlyConstructedHasZeroCount)
{
    int Dtors = 0;
    Tracer* Raw = new Tracer(&Dtors);
    EXPECT_EQ(Raw->refCount(), 0);
    Raw->addRef();
    EXPECT_EQ(Raw->refCount(), 1);
    Raw->release();
    EXPECT_EQ(Dtors, 1);
}
