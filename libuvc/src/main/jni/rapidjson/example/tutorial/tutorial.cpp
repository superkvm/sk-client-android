


#include "rapidjson/document.h"     
#include "rapidjson/prettywriter.h" 
#include "rapidjson/filestream.h"   
#include <cstdio>

using namespace rapidjson;

int main(int, char*[]) {
    
    

    const char json[] = " { \"hello\" : \"world\", \"t\" : true , \"f\" : false, \"n\": null, \"i\":123, \"pi\": 3.1416, \"a\":[1, 2, 3, 4] } ";
    printf("Original JSON:\n %s\n", json);

    Document document;  

#if 0
    
    if (document.Parse(json).HasParseError())
        return 1;
#else
    
    {
        char buffer[sizeof(json)];
        memcpy(buffer, json, sizeof(json));
        if (document.ParseInsitu(buffer).HasParseError())
            return 1;
    }
#endif

    printf("\nParsing to document succeeded.\n");

    
    

    printf("\nAccess values in document:\n");
    assert(document.IsObject());    

    assert(document.HasMember("hello"));
    assert(document["hello"].IsString());
    printf("hello = %s\n", document["hello"].GetString());

    
    Value::MemberIterator hello = document.FindMember("hello");
    assert(hello != document.MemberEnd());
    assert(hello->value.IsString());
    assert(strcmp("world", hello->value.GetString()) == 0);
    (void)hello;

    assert(document["t"].IsBool());     
    printf("t = %s\n", document["t"].GetBool() ? "true" : "false");

    assert(document["f"].IsBool());
    printf("f = %s\n", document["f"].GetBool() ? "true" : "false");

    printf("n = %s\n", document["n"].IsNull() ? "null" : "?");

    assert(document["i"].IsNumber());   
    assert(document["i"].IsInt());      
    printf("i = %d\n", document["i"].GetInt()); 

    assert(document["pi"].IsNumber());
    assert(document["pi"].IsDouble());
    printf("pi = %g\n", document["pi"].GetDouble());

    {
        const Value& a = document["a"]; 
        assert(a.IsArray());
        for (SizeType i = 0; i < a.Size(); i++) 
            printf("a[%d] = %d\n", i, a[i].GetInt());
        
        
        
        int y = a[SizeType(0)].GetInt();            
        int z = a[0u].GetInt();                     
        (void)y;
        (void)z;

        
        printf("a = ");
        for (Value::ConstValueIterator itr = a.Begin(); itr != a.End(); ++itr)
            printf("%d ", itr->GetInt());
        printf("\n");
    }

    
    static const char* kTypeNames[] = { "Null", "False", "True", "Object", "Array", "String", "Number" };
    for (Value::ConstMemberIterator itr = document.MemberBegin(); itr != document.MemberEnd(); ++itr)
        printf("Type of member %s is %s\n", itr->name.GetString(), kTypeNames[itr->value.GetType()]);

    
    

    
    {
        uint64_t f20 = 1;   
        for (uint64_t j = 1; j <= 20; j++)
            f20 *= j;
        document["i"] = f20;    
        assert(!document["i"].IsInt()); 
    }

    
    {
        Value& a = document["a"];   
        Document::AllocatorType& allocator = document.GetAllocator();
        for (int i = 5; i <= 10; i++)
            a.PushBack(i, allocator);   

        
        a.PushBack("Lua", allocator).PushBack("Mio", allocator);
    }

    

    
    
    {
        document["hello"] = "rapidjson";    
        
        
    }

    
    Value author;
    {
        char buffer[10];
        int len = sprintf(buffer, "%s %s", "Milo", "Yip");  

        author.SetString(buffer, static_cast<size_t>(len), document.GetAllocator());
        
        

        
        
        
        memset(buffer, 0, sizeof(buffer)); 
    }
    
    document.AddMember("author", author, document.GetAllocator());

    assert(author.IsNull());        

    
    

    printf("\nModified JSON with reformatting:\n");
    FileStream f(stdout);
    PrettyWriter<FileStream> writer(f);
    document.Accept(writer);    

    return 0;
}
