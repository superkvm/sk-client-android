



















#include "unittest.h"
#include "rapidjson/document.h"
#include <algorithm>

using namespace rapidjson;

TEST(Value, DefaultConstructor) {
    Value x;
    EXPECT_EQ(kNullType, x.GetType());
    EXPECT_TRUE(x.IsNull());

    
}







#if RAPIDJSON_HAS_CXX11_RVALUE_REFS
TEST(Value, MoveConstructor) {
    typedef GenericValue<UTF8<>, CrtAllocator> Value;
    Value::AllocatorType allocator;

    Value x((Value(kArrayType)));
    x.Reserve(4u, allocator);
    x.PushBack(1, allocator).PushBack(2, allocator).PushBack(3, allocator).PushBack(4, allocator);
    EXPECT_TRUE(x.IsArray());
    EXPECT_EQ(4u, x.Size());

    
    Value y(std::move(x));
    EXPECT_TRUE(x.IsNull());
    EXPECT_TRUE(y.IsArray());
    EXPECT_EQ(4u, y.Size());

    
    Value z = std::move(y);
    EXPECT_TRUE(y.IsNull());
    EXPECT_TRUE(z.IsArray());
    EXPECT_EQ(4u, z.Size());
}
#endif 

TEST(Value, AssignmentOperator) {
    Value x(1234);
    Value y;
    y = x;
    EXPECT_TRUE(x.IsNull());    
    EXPECT_EQ(1234, y.GetInt());

    y = 5678;
    EXPECT_TRUE(y.IsInt());
    EXPECT_EQ(5678, y.GetInt());

    x = "Hello";
    EXPECT_TRUE(x.IsString());
    EXPECT_STREQ(x.GetString(),"Hello");

    y = StringRef(x.GetString(),x.GetStringLength());
    EXPECT_TRUE(y.IsString());
    EXPECT_EQ(y.GetString(),x.GetString());
    EXPECT_EQ(y.GetStringLength(),x.GetStringLength());

    static char mstr[] = "mutable";
    
    y = StringRef(mstr);
    EXPECT_TRUE(y.IsString());
    EXPECT_EQ(y.GetString(),mstr);

#if RAPIDJSON_HAS_CXX11_RVALUE_REFS
    
    x = Value("World");
    EXPECT_TRUE(x.IsString());
    EXPECT_STREQ("World", x.GetString());

    x = std::move(y);
    EXPECT_TRUE(y.IsNull());
    EXPECT_TRUE(x.IsString());
    EXPECT_EQ(x.GetString(), mstr);

    y = std::move(Value().SetInt(1234));
    EXPECT_TRUE(y.IsInt());
    EXPECT_EQ(1234, y);
#endif 
}

template <typename A, typename B> 
void TestEqual(const A& a, const B& b) {
    EXPECT_TRUE (a == b);
    EXPECT_FALSE(a != b);
    EXPECT_TRUE (b == a);
    EXPECT_FALSE(b != a);
}

template <typename A, typename B> 
void TestUnequal(const A& a, const B& b) {
    EXPECT_FALSE(a == b);
    EXPECT_TRUE (a != b);
    EXPECT_FALSE(b == a);
    EXPECT_TRUE (b != a);
}

TEST(Value, EqualtoOperator) {
    Value::AllocatorType allocator;
    Value x(kObjectType);
    x.AddMember("hello", "world", allocator)
        .AddMember("t", Value(true).Move(), allocator)
        .AddMember("f", Value(false).Move(), allocator)
        .AddMember("n", Value(kNullType).Move(), allocator)
        .AddMember("i", 123, allocator)
        .AddMember("pi", 3.14, allocator)
        .AddMember("a", Value(kArrayType).Move().PushBack(1, allocator).PushBack(2, allocator).PushBack(3, allocator), allocator);

    
    TestEqual(x["hello"], "world");
    const char* cc = "world";
    TestEqual(x["hello"], cc);
    char* c = strdup("world");
    TestEqual(x["hello"], c);
    free(c);

    TestEqual(x["t"], true);
    TestEqual(x["f"], false);
    TestEqual(x["i"], 123);
    TestEqual(x["pi"], 3.14);

    
    CrtAllocator crtAllocator;
    GenericValue<UTF8<>, CrtAllocator> y;
    GenericDocument<UTF8<>, CrtAllocator> z(&crtAllocator);
    y.CopyFrom(x, crtAllocator);
    z.CopyFrom(y, z.GetAllocator());
    TestEqual(x, y);
    TestEqual(y, z);
    TestEqual(z, x);

    
    EXPECT_TRUE(y.RemoveMember("t"));
    TestUnequal(x, y);
    TestUnequal(z, y);
    EXPECT_TRUE(z.RemoveMember("t"));
    TestUnequal(x, z);
    TestEqual(y, z);
    y.AddMember("t", true, crtAllocator);
    z.AddMember("t", true, z.GetAllocator());
    TestEqual(x, y);
    TestEqual(y, z);
    TestEqual(z, x);

    
    x.SetUint64(RAPIDJSON_UINT64_C2(0xFFFFFFFF, 0xFFFFFFF0));
    y.SetUint64(RAPIDJSON_UINT64_C2(0xFFFFFFFF, 0xFFFFFFFF));
    TestUnequal(x, y);
}

template <typename Value>
void TestCopyFrom() {
    typename Value::AllocatorType a;
    Value v1(1234);
    Value v2(v1, a); 
    EXPECT_TRUE(v1.GetType() == v2.GetType());
    EXPECT_EQ(v1.GetInt(), v2.GetInt());

    v1.SetString("foo");
    v2.CopyFrom(v1, a);
    EXPECT_TRUE(v1.GetType() == v2.GetType());
    EXPECT_STREQ(v1.GetString(), v2.GetString());
    EXPECT_EQ(v1.GetString(), v2.GetString()); 

    v1.SetArray().PushBack(1234, a);
    v2.CopyFrom(v1, a);
    EXPECT_TRUE(v2.IsArray());
    EXPECT_EQ(v1.Size(), v2.Size());

    v1.PushBack(Value().SetString("foo", a), a); 
    EXPECT_TRUE(v1.Size() != v2.Size());
    v2.CopyFrom(v1, a);
    EXPECT_TRUE(v1.Size() == v2.Size());
    EXPECT_STREQ(v1[1].GetString(), v2[1].GetString());
    EXPECT_NE(v1[1].GetString(), v2[1].GetString()); 
}

TEST(Value, CopyFrom) {
    TestCopyFrom<Value>();
    TestCopyFrom<GenericValue<UTF8<>, CrtAllocator> >();
}

TEST(Value, Swap) {
    Value v1(1234);
    Value v2(kObjectType);

    EXPECT_EQ(&v1, &v1.Swap(v2));
    EXPECT_TRUE(v1.IsObject());
    EXPECT_TRUE(v2.IsInt());
    EXPECT_EQ(1234, v2.GetInt());
}

TEST(Value, Null) {
    
    Value x;
    EXPECT_EQ(kNullType, x.GetType());
    EXPECT_TRUE(x.IsNull());

    EXPECT_FALSE(x.IsTrue());
    EXPECT_FALSE(x.IsFalse());
    EXPECT_FALSE(x.IsNumber());
    EXPECT_FALSE(x.IsString());
    EXPECT_FALSE(x.IsObject());
    EXPECT_FALSE(x.IsArray());

    
    Value y(kNullType);
    EXPECT_TRUE(y.IsNull());

    
    Value z(true);
    z.SetNull();
    EXPECT_TRUE(z.IsNull());
}

TEST(Value, True) {
    
    Value x(true);
    EXPECT_EQ(kTrueType, x.GetType());
    EXPECT_TRUE(x.GetBool());
    EXPECT_TRUE(x.IsBool());
    EXPECT_TRUE(x.IsTrue());

    EXPECT_FALSE(x.IsNull());
    EXPECT_FALSE(x.IsFalse());
    EXPECT_FALSE(x.IsNumber());
    EXPECT_FALSE(x.IsString());
    EXPECT_FALSE(x.IsObject());
    EXPECT_FALSE(x.IsArray());

    
    Value y(kTrueType);
    EXPECT_TRUE(y.IsTrue());

    
    Value z;
    z.SetBool(true);
    EXPECT_TRUE(z.IsTrue());
}

TEST(Value, False) {
    
    Value x(false);
    EXPECT_EQ(kFalseType, x.GetType());
    EXPECT_TRUE(x.IsBool());
    EXPECT_TRUE(x.IsFalse());

    EXPECT_FALSE(x.IsNull());
    EXPECT_FALSE(x.IsTrue());
    EXPECT_FALSE(x.GetBool());
    
    EXPECT_FALSE(x.IsNumber());
    EXPECT_FALSE(x.IsString());
    EXPECT_FALSE(x.IsObject());
    EXPECT_FALSE(x.IsArray());

    
    Value y(kFalseType);
    EXPECT_TRUE(y.IsFalse());

    
    Value z;
    z.SetBool(false);
    EXPECT_TRUE(z.IsFalse());
}

TEST(Value, Int) {
    
    Value x(1234);
    EXPECT_EQ(kNumberType, x.GetType());
    EXPECT_EQ(1234, x.GetInt());
    EXPECT_EQ(1234u, x.GetUint());
    EXPECT_EQ(1234, x.GetInt64());
    EXPECT_EQ(1234u, x.GetUint64());
    EXPECT_EQ(1234, x.GetDouble());
    
    
    
    
    
    EXPECT_TRUE(x.IsNumber());
    EXPECT_TRUE(x.IsInt());
    EXPECT_TRUE(x.IsUint());
    EXPECT_TRUE(x.IsInt64());
    EXPECT_TRUE(x.IsUint64());

    EXPECT_FALSE(x.IsDouble());
    EXPECT_FALSE(x.IsNull());
    EXPECT_FALSE(x.IsBool());
    EXPECT_FALSE(x.IsFalse());
    EXPECT_FALSE(x.IsTrue());
    EXPECT_FALSE(x.IsString());
    EXPECT_FALSE(x.IsObject());
    EXPECT_FALSE(x.IsArray());

    Value nx(-1234);
    EXPECT_EQ(-1234, nx.GetInt());
    EXPECT_EQ(-1234, nx.GetInt64());
    EXPECT_TRUE(nx.IsInt());
    EXPECT_TRUE(nx.IsInt64());
    EXPECT_FALSE(nx.IsUint());
    EXPECT_FALSE(nx.IsUint64());

    
    Value y(kNumberType);
    EXPECT_TRUE(y.IsNumber());
    EXPECT_TRUE(y.IsInt());
    EXPECT_EQ(0, y.GetInt());

    
    Value z;
    z.SetInt(1234);
    EXPECT_EQ(1234, z.GetInt());

    
    z = 5678;
    EXPECT_EQ(5678, z.GetInt());
}

TEST(Value, Uint) {
    
    Value x(1234u);
    EXPECT_EQ(kNumberType, x.GetType());
    EXPECT_EQ(1234, x.GetInt());
    EXPECT_EQ(1234u, x.GetUint());
    EXPECT_EQ(1234, x.GetInt64());
    EXPECT_EQ(1234u, x.GetUint64());
    EXPECT_TRUE(x.IsNumber());
    EXPECT_TRUE(x.IsInt());
    EXPECT_TRUE(x.IsUint());
    EXPECT_TRUE(x.IsInt64());
    EXPECT_TRUE(x.IsUint64());
    EXPECT_EQ(1234.0, x.GetDouble());   

    EXPECT_FALSE(x.IsDouble());
    EXPECT_FALSE(x.IsNull());
    EXPECT_FALSE(x.IsBool());
    EXPECT_FALSE(x.IsFalse());
    EXPECT_FALSE(x.IsTrue());
    EXPECT_FALSE(x.IsString());
    EXPECT_FALSE(x.IsObject());
    EXPECT_FALSE(x.IsArray());

    
    Value z;
    z.SetUint(1234);
    EXPECT_EQ(1234u, z.GetUint());

    
    z = 5678u;
    EXPECT_EQ(5678u, z.GetUint());

    z = 2147483648u;    
    EXPECT_EQ(2147483648u, z.GetUint());
    EXPECT_FALSE(z.IsInt());
    EXPECT_TRUE(z.IsInt64());   
}

TEST(Value, Int64) {
    
    Value x(int64_t(1234LL));
    EXPECT_EQ(kNumberType, x.GetType());
    EXPECT_EQ(1234, x.GetInt());
    EXPECT_EQ(1234u, x.GetUint());
    EXPECT_EQ(1234, x.GetInt64());
    EXPECT_EQ(1234u, x.GetUint64());
    EXPECT_TRUE(x.IsNumber());
    EXPECT_TRUE(x.IsInt());
    EXPECT_TRUE(x.IsUint());
    EXPECT_TRUE(x.IsInt64());
    EXPECT_TRUE(x.IsUint64());

    EXPECT_FALSE(x.IsDouble());
    EXPECT_FALSE(x.IsNull());
    EXPECT_FALSE(x.IsBool());
    EXPECT_FALSE(x.IsFalse());
    EXPECT_FALSE(x.IsTrue());
    EXPECT_FALSE(x.IsString());
    EXPECT_FALSE(x.IsObject());
    EXPECT_FALSE(x.IsArray());

    Value nx(int64_t(-1234LL));
    EXPECT_EQ(-1234, nx.GetInt());
    EXPECT_EQ(-1234, nx.GetInt64());
    EXPECT_TRUE(nx.IsInt());
    EXPECT_TRUE(nx.IsInt64());
    EXPECT_FALSE(nx.IsUint());
    EXPECT_FALSE(nx.IsUint64());

    
    Value z;
    z.SetInt64(1234);
    EXPECT_EQ(1234, z.GetInt64());

    z.SetInt64(2147483648LL);   
    EXPECT_FALSE(z.IsInt());
    EXPECT_TRUE(z.IsUint());

    z.SetInt64(4294967296LL);   
    EXPECT_FALSE(z.IsInt());
    EXPECT_FALSE(z.IsUint());
}

TEST(Value, Uint64) {
    
    Value x(uint64_t(1234LL));
    EXPECT_EQ(kNumberType, x.GetType());
    EXPECT_EQ(1234, x.GetInt());
    EXPECT_EQ(1234u, x.GetUint());
    EXPECT_EQ(1234, x.GetInt64());
    EXPECT_EQ(1234u, x.GetUint64());
    EXPECT_TRUE(x.IsNumber());
    EXPECT_TRUE(x.IsInt());
    EXPECT_TRUE(x.IsUint());
    EXPECT_TRUE(x.IsInt64());
    EXPECT_TRUE(x.IsUint64());

    EXPECT_FALSE(x.IsDouble());
    EXPECT_FALSE(x.IsNull());
    EXPECT_FALSE(x.IsBool());
    EXPECT_FALSE(x.IsFalse());
    EXPECT_FALSE(x.IsTrue());
    EXPECT_FALSE(x.IsString());
    EXPECT_FALSE(x.IsObject());
    EXPECT_FALSE(x.IsArray());

    
    Value z;
    z.SetUint64(1234);
    EXPECT_EQ(1234u, z.GetUint64());

    z.SetUint64(2147483648LL);  
    EXPECT_FALSE(z.IsInt());
    EXPECT_TRUE(z.IsUint());
    EXPECT_TRUE(z.IsInt64());

    z.SetUint64(4294967296LL);  
    EXPECT_FALSE(z.IsInt());
    EXPECT_FALSE(z.IsUint());
    EXPECT_TRUE(z.IsInt64());

    z.SetUint64(9223372036854775808uLL);    
    EXPECT_FALSE(z.IsInt64());

    
    EXPECT_EQ(9223372036854775808uLL, z.GetUint64());
}

TEST(Value, Double) {
    
    Value x(12.34);
    EXPECT_EQ(kNumberType, x.GetType());
    EXPECT_EQ(12.34, x.GetDouble());
    EXPECT_TRUE(x.IsNumber());
    EXPECT_TRUE(x.IsDouble());

    EXPECT_FALSE(x.IsInt());
    EXPECT_FALSE(x.IsNull());
    EXPECT_FALSE(x.IsBool());
    EXPECT_FALSE(x.IsFalse());
    EXPECT_FALSE(x.IsTrue());
    EXPECT_FALSE(x.IsString());
    EXPECT_FALSE(x.IsObject());
    EXPECT_FALSE(x.IsArray());

    
    Value z;
    z.SetDouble(12.34);
    EXPECT_EQ(12.34, z.GetDouble());

    z = 56.78;
    EXPECT_EQ(56.78, z.GetDouble());
}

TEST(Value, String) {
    
    Value x("Hello", 5); 
    EXPECT_EQ(kStringType, x.GetType());
    EXPECT_TRUE(x.IsString());
    EXPECT_STREQ("Hello", x.GetString());
    EXPECT_EQ(5u, x.GetStringLength());

    EXPECT_FALSE(x.IsNumber());
    EXPECT_FALSE(x.IsNull());
    EXPECT_FALSE(x.IsBool());
    EXPECT_FALSE(x.IsFalse());
    EXPECT_FALSE(x.IsTrue());
    EXPECT_FALSE(x.IsObject());
    EXPECT_FALSE(x.IsArray());

    static const char cstr[] = "World"; 
    Value(cstr).Swap(x);
    EXPECT_TRUE(x.IsString());
    EXPECT_EQ(x.GetString(), cstr);
    EXPECT_EQ(x.GetStringLength(), sizeof(cstr)-1);

    static char mstr[] = "Howdy"; 
    
    Value(StringRef(mstr)).Swap(x);
    EXPECT_TRUE(x.IsString());
    EXPECT_EQ(x.GetString(), mstr);
    EXPECT_EQ(x.GetStringLength(), sizeof(mstr)-1);
    strncpy(mstr,"Hello", sizeof(mstr));
    EXPECT_STREQ(x.GetString(), "Hello");

    const char* pstr = cstr;
    
    Value(StringRef(pstr)).Swap(x);
    EXPECT_TRUE(x.IsString());
    EXPECT_EQ(x.GetString(), cstr);
    EXPECT_EQ(x.GetStringLength(), sizeof(cstr)-1);

    char* mpstr = mstr;
    Value(StringRef(mpstr,sizeof(mstr)-1)).Swap(x);
    EXPECT_TRUE(x.IsString());
    EXPECT_EQ(x.GetString(), mstr);
    EXPECT_EQ(x.GetStringLength(), 5u);
    EXPECT_STREQ(x.GetString(), "Hello");

    
    MemoryPoolAllocator<> allocator;
    Value c(x.GetString(), x.GetStringLength(), allocator);
    EXPECT_NE(x.GetString(), c.GetString());
    EXPECT_EQ(x.GetStringLength(), c.GetStringLength());
    EXPECT_STREQ(x.GetString(), c.GetString());
    
    x.SetString("World", 5);
    EXPECT_STREQ("Hello", c.GetString());
    EXPECT_EQ(5u, c.GetStringLength());

    
    Value y(kStringType);
    EXPECT_TRUE(y.IsString());
    EXPECT_EQ(0, y.GetString());
    EXPECT_EQ(0u, y.GetStringLength());

    
    Value z;
    z.SetString("Hello");
    EXPECT_TRUE(x.IsString());
    z.SetString("Hello", 5);
    EXPECT_STREQ("Hello", z.GetString());
    EXPECT_STREQ("Hello", z.GetString());
    EXPECT_EQ(5u, z.GetStringLength());

    z.SetString("Hello");
    EXPECT_TRUE(z.IsString());
    EXPECT_STREQ("Hello", z.GetString());

    
    
    z.SetString(StringRef(mstr));
    EXPECT_TRUE(z.IsString());
    EXPECT_STREQ(z.GetString(), mstr);

    z.SetString(cstr);
    EXPECT_TRUE(z.IsString());
    EXPECT_EQ(cstr, z.GetString());

    z = cstr;
    EXPECT_TRUE(z.IsString());
    EXPECT_EQ(cstr, z.GetString());

    
    char s[] = "World";
    Value w;
    w.SetString(s, (SizeType)strlen(s), allocator);
    s[0] = '\0';
    EXPECT_STREQ("World", w.GetString());
    EXPECT_EQ(5u, w.GetStringLength());

#if RAPIDJSON_HAS_STDSTRING
    {
        std::string str = "Hello World";
        str[5] = '\0';
        EXPECT_STREQ(str.data(),"Hello"); 
        EXPECT_EQ(str.size(), 11u);

        
        Value vs0(StringRef(str));
        EXPECT_TRUE(vs0.IsString());
        EXPECT_EQ(vs0.GetString(), str.data());
        EXPECT_EQ(vs0.GetStringLength(), str.size());
        TestEqual(vs0, str);

        
        Value vs1(str, allocator);
        EXPECT_TRUE(vs1.IsString());
        EXPECT_NE(vs1.GetString(), str.data());
        EXPECT_NE(vs1.GetString(), str); 
        EXPECT_EQ(vs1.GetStringLength(), str.size());
        TestEqual(vs1, str);

        
        str = "World";
        vs0.SetNull().SetString(str, allocator);
        EXPECT_TRUE(vs0.IsString());
        EXPECT_STREQ(vs0.GetString(), str.c_str());
        EXPECT_EQ(vs0.GetStringLength(), str.size());
        TestEqual(str, vs0);
        TestUnequal(str, vs1);

        
        vs1 = StringRef(str);
        TestEqual(str, vs1);
        TestEqual(vs0, vs1);
    }
#endif 
}

TEST(Value, Array) {
    Value x(kArrayType);
    const Value& y = x;
    Value::AllocatorType allocator;

    EXPECT_EQ(kArrayType, x.GetType());
    EXPECT_TRUE(x.IsArray());
    EXPECT_TRUE(x.Empty());
    EXPECT_EQ(0u, x.Size());
    EXPECT_TRUE(y.IsArray());
    EXPECT_TRUE(y.Empty());
    EXPECT_EQ(0u, y.Size());

    EXPECT_FALSE(x.IsNull());
    EXPECT_FALSE(x.IsBool());
    EXPECT_FALSE(x.IsFalse());
    EXPECT_FALSE(x.IsTrue());
    EXPECT_FALSE(x.IsString());
    EXPECT_FALSE(x.IsObject());

    
    Value v;
    x.PushBack(v, allocator);
    v.SetBool(true);
    x.PushBack(v, allocator);
    v.SetBool(false);
    x.PushBack(v, allocator);
    v.SetInt(123);
    x.PushBack(v, allocator);
    
    x.PushBack("foo", allocator);

    EXPECT_FALSE(x.Empty());
    EXPECT_EQ(5u, x.Size());
    EXPECT_FALSE(y.Empty());
    EXPECT_EQ(5u, y.Size());
    EXPECT_TRUE(x[SizeType(0)].IsNull());
    EXPECT_TRUE(x[1u].IsTrue());
    EXPECT_TRUE(x[2u].IsFalse());
    EXPECT_TRUE(x[3u].IsInt());
    EXPECT_EQ(123, x[3u].GetInt());
    EXPECT_TRUE(y[SizeType(0)].IsNull());
    EXPECT_TRUE(y[1u].IsTrue());
    EXPECT_TRUE(y[2u].IsFalse());
    EXPECT_TRUE(y[3u].IsInt());
    EXPECT_EQ(123, y[3u].GetInt());
    EXPECT_TRUE(y[4u].IsString());
    EXPECT_STREQ("foo", y[4u].GetString());

#if RAPIDJSON_HAS_CXX11_RVALUE_REFS
    
    {
        Value y(kArrayType);
        y.PushBack(Value(true), allocator);
        y.PushBack(std::move(Value(kArrayType).PushBack(Value(1), allocator).PushBack("foo", allocator)), allocator);
        EXPECT_EQ(2u, y.Size());
        EXPECT_TRUE(y[0u].IsTrue());
        EXPECT_TRUE(y[1u].IsArray());
        EXPECT_EQ(2u, y[1u].Size());
        EXPECT_TRUE(y[1u][0u].IsInt());
        EXPECT_TRUE(y[1u][1u].IsString());
    }
#endif

    
    Value::ValueIterator itr = x.Begin();
    EXPECT_TRUE(itr != x.End());
    EXPECT_TRUE(itr->IsNull());
    ++itr;
    EXPECT_TRUE(itr != x.End());
    EXPECT_TRUE(itr->IsTrue());
    ++itr;
    EXPECT_TRUE(itr != x.End());
    EXPECT_TRUE(itr->IsFalse());
    ++itr;
    EXPECT_TRUE(itr != x.End());
    EXPECT_TRUE(itr->IsInt());
    EXPECT_EQ(123, itr->GetInt());
    ++itr;
    EXPECT_TRUE(itr != x.End());
    EXPECT_TRUE(itr->IsString());
    EXPECT_STREQ("foo", itr->GetString());

    
    Value::ConstValueIterator citr = y.Begin();
    EXPECT_TRUE(citr != y.End());
    EXPECT_TRUE(citr->IsNull());
    ++citr;
    EXPECT_TRUE(citr != y.End());
    EXPECT_TRUE(citr->IsTrue());
    ++citr;
    EXPECT_TRUE(citr != y.End());
    EXPECT_TRUE(citr->IsFalse());
    ++citr;
    EXPECT_TRUE(citr != y.End());
    EXPECT_TRUE(citr->IsInt());
    EXPECT_EQ(123, citr->GetInt());
    ++citr;
    EXPECT_TRUE(citr != y.End());
    EXPECT_TRUE(citr->IsString());
    EXPECT_STREQ("foo", citr->GetString());

    
    x.PopBack();
    EXPECT_EQ(4u, x.Size());
    EXPECT_TRUE(y[SizeType(0)].IsNull());
    EXPECT_TRUE(y[1u].IsTrue());
    EXPECT_TRUE(y[2u].IsFalse());
    EXPECT_TRUE(y[3u].IsInt());

    
    x.Clear();
    EXPECT_TRUE(x.Empty());
    EXPECT_EQ(0u, x.Size());
    EXPECT_TRUE(y.Empty());
    EXPECT_EQ(0u, y.Size());

    

    
    
    for (int i = 0; i < 10; i++)
        x.PushBack(Value(kArrayType).PushBack(i, allocator).Move(), allocator);

    
    itr = x.Erase(x.Begin());
    EXPECT_EQ(x.Begin(), itr);
    EXPECT_EQ(9u, x.Size());
    for (int i = 0; i < 9; i++)
        EXPECT_EQ(i + 1, x[i][0u].GetInt());

    
    itr = x.Erase(x.End() - 1);
    EXPECT_EQ(x.End(), itr);
    EXPECT_EQ(8u, x.Size());
    for (int i = 0; i < 8; i++)
        EXPECT_EQ(i + 1, x[i][0u].GetInt());

    
    itr = x.Erase(x.Begin() + 4);
    EXPECT_EQ(x.Begin() + 4, itr);
    EXPECT_EQ(7u, x.Size());
    for (int i = 0; i < 4; i++)
        EXPECT_EQ(i + 1, x[i][0u].GetInt());
    for (int i = 4; i < 7; i++)
        EXPECT_EQ(i + 2, x[i][0u].GetInt());

    
    
    const unsigned n = 10;
    for (unsigned first = 0; first < n; first++) {
        for (unsigned last = first; last <= n; last++) {
            x.Clear();
            for (unsigned i = 0; i < n; i++)
                x.PushBack(Value(kArrayType).PushBack(i, allocator).Move(), allocator);
            
            itr = x.Erase(x.Begin() + first, x.Begin() + last);
            if (last == n)
                EXPECT_EQ(x.End(), itr);
            else
                EXPECT_EQ(x.Begin() + first, itr);

            size_t removeCount = last - first;
            EXPECT_EQ(n - removeCount, x.Size());
            for (unsigned i = 0; i < first; i++)
                EXPECT_EQ(i, x[i][0u].GetUint());
            for (unsigned i = first; i < n - removeCount; i++)
                EXPECT_EQ(i + removeCount, x[i][0u].GetUint());
        }
    }

    
    
    x.Clear();
    for (int i = 0; i < 10; i++)
        if (i % 2 == 0)
            x.PushBack(i, allocator);
        else
            x.PushBack(Value(kNullType).Move(), allocator);

    const Value null(kNullType);
    x.Erase(std::remove(x.Begin(), x.End(), null), x.End());
    EXPECT_EQ(5u, x.Size());
    for (int i = 0; i < 5; i++)
        EXPECT_EQ(i * 2, x[i]);

    
    Value z;
    z.SetArray();
    EXPECT_TRUE(z.IsArray());
    EXPECT_TRUE(z.Empty());
}

TEST(Value, Object) {
    Value x(kObjectType);
    const Value& y = x; 
    Value::AllocatorType allocator;

    EXPECT_EQ(kObjectType, x.GetType());
    EXPECT_TRUE(x.IsObject());
    EXPECT_TRUE(x.ObjectEmpty());
    EXPECT_EQ(0u, x.MemberCount());
    EXPECT_EQ(kObjectType, y.GetType());
    EXPECT_TRUE(y.IsObject());
    EXPECT_TRUE(y.ObjectEmpty());
    EXPECT_EQ(0u, y.MemberCount());

    
    x.AddMember("A", "Apple", allocator);
    EXPECT_FALSE(x.ObjectEmpty());
    EXPECT_EQ(1u, x.MemberCount());

    Value value("Banana", 6);
    x.AddMember("B", "Banana", allocator);
    EXPECT_EQ(2u, x.MemberCount());

    
    {
        Value o(kObjectType);
        o.AddMember("true", true, allocator);
        o.AddMember("false", false, allocator);
        o.AddMember("int", -1, allocator);
        o.AddMember("uint", 1u, allocator);
        o.AddMember("int64", INT64_C(-4294967296), allocator);
        o.AddMember("uint64", UINT64_C(4294967296), allocator);
        o.AddMember("double", 3.14, allocator);
        o.AddMember("string", "Jelly", allocator);

        EXPECT_TRUE(o["true"].GetBool());
        EXPECT_FALSE(o["false"].GetBool());
        EXPECT_EQ(-1, o["int"].GetInt());
        EXPECT_EQ(1u, o["uint"].GetUint());
        EXPECT_EQ(INT64_C(-4294967296), o["int64"].GetInt64());
        EXPECT_EQ(UINT64_C(4294967296), o["uint64"].GetUint64());
        EXPECT_STREQ("Jelly",o["string"].GetString());
        EXPECT_EQ(8u, o.MemberCount());
    }

#if RAPIDJSON_HAS_CXX11_RVALUE_REFS
    
    {
        Value o(kObjectType);
        o.AddMember(Value("true"), Value(true), allocator);
        o.AddMember(Value("false"), Value(false).Move(), allocator);    
        o.AddMember(Value("int").Move(), Value(-1), allocator);         
        o.AddMember("uint", std::move(Value().SetUint(1u)), allocator); 
        EXPECT_TRUE(o["true"].GetBool());
        EXPECT_FALSE(o["false"].GetBool());
        EXPECT_EQ(-1, o["int"].GetInt());
        EXPECT_EQ(1u, o["uint"].GetUint());
        EXPECT_EQ(4u, o.MemberCount());
    }
#endif

    
    Value name;
    const Value C0D("C\0D", 3);
    name.SetString(C0D.GetString(), 3);
    value.SetString("CherryD", 7);
    x.AddMember(name, value, allocator);

    
    EXPECT_TRUE(x.HasMember("A"));
    EXPECT_TRUE(x.HasMember("B"));
    EXPECT_TRUE(y.HasMember("A"));
    EXPECT_TRUE(y.HasMember("B"));

    name.SetString("C\0D");
    EXPECT_TRUE(x.HasMember(name));
    EXPECT_TRUE(y.HasMember(name));

    GenericValue<UTF8<>, CrtAllocator> othername("A");
    EXPECT_TRUE(x.HasMember(othername));
    EXPECT_TRUE(y.HasMember(othername));
    othername.SetString("C\0D");
    EXPECT_TRUE(x.HasMember(othername));
    EXPECT_TRUE(y.HasMember(othername));

    
    EXPECT_STREQ("Apple", x["A"].GetString());
    EXPECT_STREQ("Banana", x["B"].GetString());
    EXPECT_STREQ("CherryD", x[C0D].GetString());
    EXPECT_STREQ("CherryD", x[othername].GetString());

    
    EXPECT_STREQ("Apple", y["A"].GetString());
    EXPECT_STREQ("Banana", y["B"].GetString());
    EXPECT_STREQ("CherryD", y[C0D].GetString());

    
    Value::MemberIterator itr = x.MemberBegin(); 
    EXPECT_TRUE(itr != x.MemberEnd());
    EXPECT_STREQ("A", itr->name.GetString());
    EXPECT_STREQ("Apple", itr->value.GetString());
    ++itr;
    EXPECT_TRUE(itr != x.MemberEnd());
    EXPECT_STREQ("B", itr->name.GetString());
    EXPECT_STREQ("Banana", itr->value.GetString());
    ++itr;
    EXPECT_TRUE(itr != x.MemberEnd());
    EXPECT_TRUE(memcmp(itr->name.GetString(), "C\0D", 4) == 0);
    EXPECT_STREQ("CherryD", itr->value.GetString());
    ++itr;
    EXPECT_FALSE(itr != x.MemberEnd());

    
    Value::ConstMemberIterator citr = y.MemberBegin(); 
    EXPECT_TRUE(citr != y.MemberEnd());
    EXPECT_STREQ("A", citr->name.GetString());
    EXPECT_STREQ("Apple", citr->value.GetString());
    ++citr;
    EXPECT_TRUE(citr != y.MemberEnd());
    EXPECT_STREQ("B", citr->name.GetString());
    EXPECT_STREQ("Banana", citr->value.GetString());
    ++citr;
    EXPECT_TRUE(citr != y.MemberEnd());
    EXPECT_TRUE(memcmp(citr->name.GetString(), "C\0D", 4) == 0);
    EXPECT_STREQ("CherryD", citr->value.GetString());
    ++citr;
    EXPECT_FALSE(citr != y.MemberEnd());

    
    itr  = x.MemberBegin();
    citr = x.MemberBegin(); 
    TestEqual(itr, citr);
    EXPECT_TRUE(itr < x.MemberEnd());
    EXPECT_FALSE(itr > y.MemberEnd());
    EXPECT_TRUE(citr < x.MemberEnd());
    EXPECT_FALSE(citr > y.MemberEnd());
    ++citr;
    TestUnequal(itr, citr);
    EXPECT_FALSE(itr < itr);
    EXPECT_TRUE(itr < citr);
    EXPECT_FALSE(itr > itr);
    EXPECT_TRUE(citr > itr);
    EXPECT_EQ(1, citr - x.MemberBegin());
    EXPECT_EQ(0, itr - y.MemberBegin());
    itr += citr - x.MemberBegin();
    EXPECT_EQ(1, itr - y.MemberBegin());
    TestEqual(citr, itr);
    EXPECT_TRUE(itr <= citr);
    EXPECT_TRUE(citr <= itr);
    itr++;
    EXPECT_TRUE(itr >= citr);
    EXPECT_FALSE(citr >= itr);

    
    x.RemoveMember("A");
    EXPECT_FALSE(x.HasMember("A"));

    x.RemoveMember("B");
    EXPECT_FALSE(x.HasMember("B"));

    x.RemoveMember(othername);
    EXPECT_FALSE(x.HasMember(name));

    EXPECT_TRUE(x.MemberBegin() == x.MemberEnd());

    

    
    
    const char keys[][2] = { "a", "b", "c", "d", "e", "f", "g", "h", "i", "j" };
    for (int i = 0; i < 10; i++)
        x.AddMember(keys[i], Value(kArrayType).PushBack(i, allocator), allocator);

    
    EXPECT_EQ(x.MemberCount(), SizeType(x.MemberEnd() - x.MemberBegin()));

    
    itr = x.EraseMember(x.MemberBegin());
    EXPECT_FALSE(x.HasMember(keys[0]));
    EXPECT_EQ(x.MemberBegin(), itr);
    EXPECT_EQ(9u, x.MemberCount());
    for (; itr != x.MemberEnd(); ++itr) {
        int i = (itr - x.MemberBegin()) + 1;
        EXPECT_STREQ(itr->name.GetString(), keys[i]);
        EXPECT_EQ(i, itr->value[0u].GetInt());
    }

    
    itr = x.EraseMember(x.MemberEnd() - 1);
    EXPECT_FALSE(x.HasMember(keys[9]));
    EXPECT_EQ(x.MemberEnd(), itr);
    EXPECT_EQ(8u, x.MemberCount());
    for (; itr != x.MemberEnd(); ++itr) {
        int i = (itr - x.MemberBegin()) + 1;
        EXPECT_STREQ(itr->name.GetString(), keys[i]);
        EXPECT_EQ(i, itr->value[0u].GetInt());
    }

    
    itr = x.EraseMember(x.MemberBegin() + 4);
    EXPECT_FALSE(x.HasMember(keys[5]));
    EXPECT_EQ(x.MemberBegin() + 4, itr);
    EXPECT_EQ(7u, x.MemberCount());
    for (; itr != x.MemberEnd(); ++itr) {
        int i = (itr - x.MemberBegin());
        i += (i<4) ? 1 : 2;
        EXPECT_STREQ(itr->name.GetString(), keys[i]);
        EXPECT_EQ(i, itr->value[0u].GetInt());
    }

    
    
    const unsigned n = 10;
    for (unsigned first = 0; first < n; first++) {
        for (unsigned last = first; last <= n; last++) {
            Value(kObjectType).Swap(x);
            for (unsigned i = 0; i < n; i++)
                x.AddMember(keys[i], Value(kArrayType).PushBack(i, allocator), allocator);

            itr = x.EraseMember(x.MemberBegin() + first, x.MemberBegin() + last);
            if (last == n)
                EXPECT_EQ(x.MemberEnd(), itr);
            else
                EXPECT_EQ(x.MemberBegin() + first, itr);

            size_t removeCount = last - first;
            EXPECT_EQ(n - removeCount, x.MemberCount());
            for (unsigned i = 0; i < first; i++)
                EXPECT_EQ(i, x[keys[i]][0u].GetUint());
            for (unsigned i = first; i < n - removeCount; i++)
                EXPECT_EQ(i + removeCount, x[keys[i+removeCount]][0u].GetUint());
        }
    }

    
    x.RemoveAllMembers();
    EXPECT_TRUE(x.ObjectEmpty());
    EXPECT_EQ(0u, x.MemberCount());

    
    Value z;
    z.SetObject();
    EXPECT_TRUE(z.IsObject());
}

TEST(Value, BigNestedArray) {
    MemoryPoolAllocator<> allocator;
    Value x(kArrayType);
    static const SizeType  n = 200;

    for (SizeType i = 0; i < n; i++) {
        Value y(kArrayType);
        for (SizeType  j = 0; j < n; j++) {
            Value number((int)(i * n + j));
            y.PushBack(number, allocator);
        }
        x.PushBack(y, allocator);
    }

    for (SizeType i = 0; i < n; i++)
        for (SizeType j = 0; j < n; j++) {
            EXPECT_TRUE(x[i][j].IsInt());
            EXPECT_EQ((int)(i * n + j), x[i][j].GetInt());
        }
}

TEST(Value, BigNestedObject) {
    MemoryPoolAllocator<> allocator;
    Value x(kObjectType);
    static const SizeType n = 200;

    for (SizeType i = 0; i < n; i++) {
        char name1[10];
        sprintf(name1, "%d", i);

        
        Value name(name1, (SizeType)strlen(name1), allocator);
        Value object(kObjectType);

        for (SizeType j = 0; j < n; j++) {
            char name2[10];
            sprintf(name2, "%d", j);

            Value name(name2, (SizeType)strlen(name2), allocator);
            Value number((int)(i * n + j));
            object.AddMember(name, number, allocator);
        }

        
        x.AddMember(name, object, allocator);
    }

    for (SizeType i = 0; i < n; i++) {
        char name1[10];
        sprintf(name1, "%d", i);
        
        for (SizeType j = 0; j < n; j++) {
            char name2[10];
            sprintf(name2, "%d", j);
            x[name1];
            EXPECT_EQ((int)(i * n + j), x[name1][name2].GetInt());
        }
    }
}



TEST(Value, RemoveLastElement) {
    rapidjson::Document doc;
    rapidjson::Document::AllocatorType& allocator = doc.GetAllocator();
    rapidjson::Value objVal(rapidjson::kObjectType);        
    objVal.AddMember("var1", 123, allocator);       
    objVal.AddMember("var2", "444", allocator);
    objVal.AddMember("var3", 555, allocator);
    EXPECT_TRUE(objVal.HasMember("var3"));
    objVal.RemoveMember("var3");    
    EXPECT_FALSE(objVal.HasMember("var3"));
}


TEST(Document, CrtAllocator) {
    typedef GenericValue<UTF8<>, CrtAllocator> V;

    V::AllocatorType allocator;
    V o(kObjectType);
    o.AddMember("x", 1, allocator); 

    V a(kArrayType);
    a.PushBack(1, allocator);   
}

static void TestShortStringOptimization(const char* str) {
    const rapidjson::SizeType len = (rapidjson::SizeType)strlen(str);
	
    rapidjson::Document doc;
    rapidjson::Value val;
    val.SetString(str, len, doc.GetAllocator());
	
	EXPECT_EQ(val.GetStringLength(), len);
	EXPECT_STREQ(val.GetString(), str);
}

TEST(Value, AllocateShortString) {
	TestShortStringOptimization("");                 
	TestShortStringOptimization("12345678");         
	TestShortStringOptimization("12345678901");      
	TestShortStringOptimization("123456789012");     
	TestShortStringOptimization("123456789012345");  
	TestShortStringOptimization("1234567890123456"); 
}
