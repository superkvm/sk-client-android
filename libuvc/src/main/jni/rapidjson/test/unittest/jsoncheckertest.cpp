



















#include "unittest.h"

#include "rapidjson/document.h"

using namespace rapidjson;

static char* ReadFile(const char* filename, size_t& length) {
    FILE *fp = fopen(filename, "rb");
    if (!fp)
        fp = fopen(filename, "rb");
    if (!fp)
        return 0;

    fseek(fp, 0, SEEK_END);
    length = (size_t)ftell(fp);
    fseek(fp, 0, SEEK_SET);
    char* json = (char*)malloc(length + 1);
    size_t readLength = fread(json, 1, length, fp);
    json[readLength] = '\0';
    fclose(fp);
    return json;
}

TEST(JsonChecker, Reader) {
    char filename[256];

    
    for (int i = 1; i <= 33; i++) {
        if (i == 1) 
            continue;
        if (i == 18)    
            continue;

        sprintf(filename, "jsonchecker/fail%d.json", i);
        size_t length;
        char* json = ReadFile(filename, length);
        if (!json) {
            sprintf(filename, "../../bin/jsonchecker/fail%d.json", i);
            json = ReadFile(filename, length);
            if (!json) {
                printf("jsonchecker file %s not found", filename);
                ADD_FAILURE();
                continue;
            }
        }

        GenericDocument<UTF8<>, CrtAllocator> document; 
        document.Parse((const char*)json);
        EXPECT_TRUE(document.HasParseError());

        document.Parse<kParseIterativeFlag>((const char*)json);
        EXPECT_TRUE(document.HasParseError());

        free(json);
    }

    
    for (int i = 1; i <= 3; i++) {
        sprintf(filename, "jsonchecker/pass%d.json", i);
        size_t length;
        char* json = ReadFile(filename, length);
        if (!json) {
            sprintf(filename, "../../bin/jsonchecker/pass%d.json", i);
            json = ReadFile(filename, length);
            if (!json) {
                printf("jsonchecker file %s not found", filename);
                continue;
            }
        }

        GenericDocument<UTF8<>, CrtAllocator> document; 
        document.Parse((const char*)json);
        EXPECT_FALSE(document.HasParseError());

        document.Parse<kParseIterativeFlag>((const char*)json);
        EXPECT_FALSE(document.HasParseError());

        free(json);
    }
}
