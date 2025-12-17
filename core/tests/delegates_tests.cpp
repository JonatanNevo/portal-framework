//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include <array>
#include <gtest/gtest.h>

#include "portal/core/delegates/delegate.h"
#include "portal/core/delegates/lambda_delegate.h"
#include "portal/core/delegates/raw_delegate.h"
#include "portal/core/delegates/shared_pointer_delegate.h"
#include "portal/core/delegates/static_delegate.h"

struct Foo
{
    float bar(float a)
    {
        return a;
    }

    float bar_const(float a) const
    {
        return a;
    }

    static float bar_static(float a)
    {
        return a;
    }
};

struct DelegateGenerator
{
    template <typename Callback>
    static std::unique_ptr<portal::DelegateInterface<float, float>> get_delegate(Callback&& callback)
    {
        return std::make_unique<portal::LambdaDelegate<Callback, float(float)>>(std::forward<Callback>(callback));
    }
};


TEST(InnerDelegates, StaticDelegate)
{
    portal::StaticDelegate<float(float)> delegate(&Foo::bar_static);
    EXPECT_EQ(delegate.get_owner(), nullptr);
    EXPECT_EQ(delegate.execute(10), 10);
}

TEST(InnerDelegates, LambdaDelegate)
{
    const auto delegate = DelegateGenerator::get_delegate([](float a) { return a; });
    EXPECT_EQ(delegate->get_owner(), nullptr);
    EXPECT_EQ(delegate->execute(10), 10);
}

TEST(InnerDelegates, RawConstDelegate)
{
    Foo foo;
    portal::RawDelegate<true, Foo, float(float)> delegate(&foo, &Foo::bar_const);
    EXPECT_EQ(delegate.get_owner(), &foo);
    EXPECT_EQ(delegate.execute(10), 10);
}

TEST(InnerDelegates, RawDelegate)
{
    Foo foo;
    portal::RawDelegate<false, Foo, float(float)> delegate(&foo, &Foo::bar);
    EXPECT_EQ(delegate.get_owner(), &foo);
    EXPECT_EQ(delegate.execute(10), 10);
}

TEST(InnerDelegates, SharedPtrDelegateConst)
{
    std::shared_ptr<Foo> foo = std::make_shared<Foo>();
    portal::SharedPointerDelegate<true, Foo, float(float)> delegate(foo, &Foo::bar_const);
    EXPECT_EQ(delegate.get_owner(), foo.get());
    EXPECT_EQ(delegate.execute(10), 10);
}

TEST(InnerDelegates, SharedPtrDelegate)
{
    std::shared_ptr<Foo> foo = std::make_shared<Foo>();
    portal::SharedPointerDelegate<false, Foo, float(float)> delegate(foo, &Foo::bar);
    EXPECT_EQ(delegate.get_owner(), foo.get());
    EXPECT_EQ(delegate.execute(10), 10);
}

TEST(Delegate, DelegateDefaultConstructor)
{
    portal::Delegate<void> delegate;
    EXPECT_FALSE(delegate.is_bound());
    EXPECT_EQ(delegate.get_size(), 0);
    EXPECT_EQ(delegate.get_owner(), nullptr);
}

TEST(Delegate, Constructor)
{
    portal::Delegate<void> delegate;
    delegate.bind_lambda([]() {});
    EXPECT_TRUE(delegate.is_bound());
    EXPECT_GT(delegate.get_size(), 0);
}

TEST(Delegate, CopyConstructor)
{
    portal::Delegate<void> delegate;
    delegate.bind_lambda([]() {});
    const auto second_delegate = delegate;
    EXPECT_TRUE(second_delegate.is_bound());
    EXPECT_GT(second_delegate.get_size(), 0);
    EXPECT_EQ(delegate.get_size(), second_delegate.get_size());
}

TEST(Delegate, AssignmentOperator)
{
    portal::Delegate<void> delegate;
    delegate.bind_lambda([]() {});
    portal::Delegate<void> second_delegate;
    second_delegate = delegate;
    EXPECT_TRUE(second_delegate.is_bound());
    EXPECT_GT(second_delegate.get_size(), 0);
    EXPECT_EQ(delegate.get_size(), second_delegate.get_size());
}

TEST(Delegate, MoveConstructor)
{
    portal::Delegate<void> delegate;
    delegate.bind_lambda([]() {});
    portal::Delegate<void> second_delegate(std::move(delegate));
    EXPECT_TRUE(second_delegate.is_bound());
    EXPECT_GT(second_delegate.get_size(), 0);
    EXPECT_FALSE(delegate.is_bound());
    EXPECT_EQ(delegate.get_size(), 0);
}

TEST(Delegate, MoveAssignmentOperator)
{
    portal::Delegate<void> delegate;
    delegate.bind_lambda([]() {});
    portal::Delegate<void> second_delegate;
    second_delegate = std::move(delegate);
    EXPECT_TRUE(second_delegate.is_bound());
    EXPECT_GT(second_delegate.get_size(), 0);
    EXPECT_FALSE(delegate.is_bound());
    EXPECT_EQ(delegate.get_size(), 0);
}

TEST(Delegate, CreateLambda)
{
    auto created = portal::Delegate<float, float>::create_lambda([](float a) { return a; });
    EXPECT_TRUE(created.is_bound());
    EXPECT_EQ(created.execute(10), 10);

    std::array<float, 1024> arr{0};
    auto created_large = portal::Delegate<float, float>::create_lambda(
        [arr](float a) mutable
        {
            arr[0] = a;
            return a;
        }
    );
    EXPECT_TRUE(created_large.is_bound());
    EXPECT_EQ(created_large.execute(10), 10);
}

TEST(Delegate, CreateStatic)
{
    auto created = portal::Delegate<float, float>::create_static(&Foo::bar_static);
    EXPECT_TRUE(created.is_bound());
    EXPECT_EQ(created.execute(10), 10);
}

TEST(Delegate, CreateRawConst)
{
    Foo foo;
    auto created = portal::Delegate<float, float>::create_raw(&foo, &Foo::bar_const);
    EXPECT_TRUE(created.is_bound());
    EXPECT_EQ(created.execute(10), 10);
}

TEST(Delegate, CreateRaw)
{
    Foo foo;
    auto created = portal::Delegate<float, float>::create_raw(&foo, &Foo::bar);
    EXPECT_TRUE(created.is_bound());
    EXPECT_EQ(created.execute(10), 10);
}

TEST(Delegate, CreateSharedPtrConst)
{
    auto foo = std::make_shared<Foo>();
    auto created = portal::Delegate<float, float>::create_shared_ptr(foo, &Foo::bar_const);
    EXPECT_TRUE(created.is_bound());
    EXPECT_EQ(created.execute(10), 10);
}

TEST(Delegate, CreateSharedPtr)
{
    auto foo = std::make_shared<Foo>();
    auto created = portal::Delegate<float, float>::create_shared_ptr(foo, &Foo::bar);
    EXPECT_TRUE(created.is_bound());
    EXPECT_EQ(created.execute(10), 10);
}

TEST(Delegate, BindLambda)
{
    portal::Delegate<float, float> delegate;
    delegate.bind_lambda([](float a) { return a; });
    EXPECT_TRUE(delegate.is_bound());
    EXPECT_EQ(delegate.execute(10), 10);
}

TEST(Delegate, BindStatic)
{
    portal::Delegate<float, float> delegate;
    delegate.bind_static(&Foo::bar_static);
    EXPECT_TRUE(delegate.is_bound());
    EXPECT_EQ(delegate.execute(10), 10);
}

TEST(Delegate, BindRawConst)
{
    Foo foo;
    portal::Delegate<float, float> delegate;
    delegate.bind_raw(&foo, &Foo::bar_const);
    EXPECT_TRUE(delegate.is_bound());
    EXPECT_EQ(delegate.execute(10), 10);
}

TEST(Delegate, BindRaw)
{
    Foo foo;
    portal::Delegate<float, float> delegate;
    delegate.bind_raw(&foo, &Foo::bar);
    EXPECT_TRUE(delegate.is_bound());
    EXPECT_EQ(delegate.execute(10), 10);
}

TEST(Delegate, BindSharedPtrConst)
{
    auto foo = std::make_shared<Foo>();
    portal::Delegate<float, float> delegate;
    delegate.bind_shared_ptr(foo, &Foo::bar_const);
    EXPECT_TRUE(delegate.is_bound());
    EXPECT_EQ(delegate.execute(10), 10);
}

TEST(Delegate, BindSharedPtr)
{
    auto foo = std::make_shared<Foo>();
    portal::Delegate<float, float> delegate;
    delegate.bind_shared_ptr(foo, &Foo::bar);
    EXPECT_TRUE(delegate.is_bound());
    EXPECT_EQ(delegate.execute(10), 10);
}

TEST(MulticastDelegate, AddLambdaReference)
{
    portal::MulticastDelegate<int> delegate;
    std::array<int, 1024> output{0};

    for (auto& value : output)
    {
        delegate.add_lambda([&value](int a) mutable { value = a; });
    }

    for (auto& value : output)
    {
        EXPECT_EQ(value, 0);
    }

    delegate.broadcast(10);

    for (auto& value : output)
    {
        EXPECT_EQ(value, 10);
    }
}

TEST(MulticastDelegate, AddLambdaValue)
{
    portal::MulticastDelegate<int> delegate;
    std::array<int, 1024> output{0};
    delegate.add_lambda([output](int a) mutable { output[a] = a; });

    EXPECT_EQ(output[10], 0);
    delegate.broadcast(10);
    EXPECT_EQ(output[10], 0);
}

static int a, b, c, d;

void foo_a(int _a) { a = _a; }
void foo_b(int _b) { b = _b; }
void foo_c(int _c) { c = _c; }
void foo_d(int _d) { d = _d; }

TEST(MulticastDelegate, AddStatic)
{
    a = 0;
    b = 0;
    c = 0;
    d = 0;

    portal::MulticastDelegate<int> delegate;
    delegate.add_static(foo_a);
    delegate.add_static(foo_b);
    delegate.add_static(foo_c);
    delegate.add_static(foo_d);

    delegate.broadcast(10);
    EXPECT_EQ(a, 10);
    EXPECT_EQ(b, 10);
    EXPECT_EQ(c, 10);
    EXPECT_EQ(d, 10);
}

TEST(MulticastDelegate, AddRawConst)
{
    struct Foo
    {
        Foo(std::array<int, 64>& v) : values(v) {}

        void Bar(int a) const
        {
            values[a] = a;
        }

        std::array<int, 64>& values;
    };


    std::array<int, 64> values{0};
    Foo foo(values);
    portal::MulticastDelegate<int> delegate;
    delegate.add_raw(&foo, &Foo::Bar);

    EXPECT_EQ(values[10], 0);
    delegate.broadcast(10);
    EXPECT_EQ(values[10], 10);
}

TEST(MulticastDelegate, AddRaw)
{
    struct Foo
    {
        Foo(std::array<int, 64>& v) : values(v) {}

        void Bar(int a)
        {
            values[a] = a;
        }

        std::array<int, 64>& values;
    };

    std::array<int, 64> values{0};
    Foo foo(values);
    portal::MulticastDelegate<int> delegate;
    delegate.add_raw(&foo, &Foo::Bar);

    EXPECT_EQ(values[10], 0);
    delegate.broadcast(10);
    EXPECT_EQ(values[10], 10);
}

TEST(MulticastDelegate, AddSharedPtrConst)
{
    struct Foo
    {
        Foo(std::array<int, 64>& v) : values(v) {}

        void Bar(int a) const
        {
            values[a] = a;
        }

        std::array<int, 64>& values;
    };

    std::array<int, 64> values{0};
    auto foo = std::make_shared<Foo>(values);
    portal::MulticastDelegate<int> delegate;
    delegate.add_shared_ptr(foo, &Foo::Bar);

    EXPECT_EQ(values[10], 0);
    delegate.broadcast(10);
    EXPECT_EQ(values[10], 10);
}

TEST(MulticastDelegate, AddSharedPtr)
{
    struct Foo
    {
        Foo(std::array<int, 64>& v) : values(v) {}

        void Bar(int a)
        {
            values[a] = a;
        }

        std::array<int, 64>& values;
    };

    std::array<int, 64> values{0};
    auto foo = std::make_shared<Foo>(values);
    portal::MulticastDelegate<int> delegate;
    delegate.add_shared_ptr(foo, &Foo::Bar);
    EXPECT_EQ(values[10], 0);
    delegate.broadcast(10);
    EXPECT_EQ(values[10], 10);
}

TEST(MulticastDelegate, RemoveByHandle)
{
    portal::MulticastDelegate<int> delegate;
    std::array<int, 64> values{0};

    auto handle = delegate.add_lambda(
        [&values](int a)
        {
            values[a] = a;
        }
    );

    EXPECT_EQ(values[10], 0);
    delegate.broadcast(10);
    EXPECT_EQ(values[10], 10);

    EXPECT_TRUE(delegate.remove(handle));
    delegate.broadcast(20);
    EXPECT_EQ(values[10], 10); // Value shouldn't change after removal
}

TEST(MulticastDelegate, RemoveByObject)
{
    portal::MulticastDelegate<int> delegate;
    std::array<int, 64> values{0};

    struct Foo
    {
        Foo(std::array<int, 64>& v) : values(v) {}

        void Bar(int a)
        {
            values[a] = a;
        }

        std::array<int, 64>& values;
    };

    Foo foo(values);
    delegate.add_raw(&foo, &Foo::Bar);

    EXPECT_EQ(values[10], 0);
    delegate.broadcast(10);
    EXPECT_EQ(values[10], 10);

    delegate.remove_object(&foo);
    delegate.broadcast(20);
    EXPECT_EQ(values[10], 10); // Value shouldn't change after removal
}

TEST(MulticastDelegate, DefaultConstructor)
{
    portal::MulticastDelegate<> delegate;
    EXPECT_EQ(delegate.get_count(), 0);
}

TEST(MulticastDelegate, AddMultipleLambdas)
{
    portal::MulticastDelegate<> delegate;
    delegate.add_lambda([]() {});
    delegate.add_lambda([]() {});
    delegate.add_lambda([]() {});
    EXPECT_EQ(delegate.get_count(), 3);
}

TEST(MulticastDelegate, MoveConstructor)
{
    portal::MulticastDelegate<> delegate;
    delegate.add_lambda([]() {});
    delegate.add_lambda([]() {});
    delegate.add_lambda([]() {});

    portal::MulticastDelegate<> second_delegate(std::move(delegate));
    EXPECT_EQ(second_delegate.get_count(), 3);
    EXPECT_EQ(delegate.get_count(), 0);
}

TEST(MulticastDelegate, MoveAssignmentOperator)
{
    portal::MulticastDelegate<> delegate;
    delegate.add_lambda([]() {});
    delegate.add_lambda([]() {});
    delegate.add_lambda([]() {});

    portal::MulticastDelegate<> second_delegate;
    second_delegate = std::move(delegate);

    EXPECT_EQ(second_delegate.get_count(), 3);
    EXPECT_EQ(delegate.get_count(), 0);
}
