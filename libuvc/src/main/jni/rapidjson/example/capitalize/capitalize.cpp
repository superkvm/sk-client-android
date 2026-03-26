




#include "rapidjson/reader.h"
#include "rapidjson/writer.h"
#include "rapidjson/filereadstream.h"
#include "rapidjson/filewritestream.h"
#include "rapidjson/error/en.h"
#include <vector>
#include <cctype>

using namespace rapidjson;

template<typename OutputHandler>
struct CapitalizeFilter {
    CapitalizeFilter(OutputHandler& out) : out_(out), buffer_() {}

    bool Null() { return out_.Null(); }
    bool Bool(bool b) { return out_.Bool(b); }
    bool Int(int i) { return out_.Int(i); }
    bool Uint(unsigned u) { return out_.Uint(u); }
    bool Int64(int64_t i) { return out_.Int64(i); }
    bool Uint64(uint64_t u) { return out_.Uint64(u); }
    bool Double(double d) { return out_.Double(d); }
    bool String(const char* str, SizeType length, bool) { 
        buffer_.clear();
        for (SizeType i = 0; i < length; i++)
            buffer_.push_back(std::toupper(str[i]));
        return out_.String(&buffer_.front(), length, true); 
    }
    bool StartObject() { return out_.StartObject(); }
    bool Key(const char* str, SizeType length, bool copy) { return String(str, length, copy); }
    bool EndObject(SizeType memberCount) { return out_.EndObject(memberCount); }
    bool StartArray() { return out_.StartArray(); }
    bool EndArray(SizeType elementCount) { return out_.EndArray(elementCount); }

    OutputHandler& out_;
    std::vector<char> buffer_;

private:
    CapitalizeFilter(const CapitalizeFilter&);
    CapitalizeFilter& operator=(const CapitalizeFilter&);
};

int main(int, char*[]) {
    
    Reader reader;
    char readBuffer[65536];
    FileReadStream is(stdin, readBuffer, sizeof(readBuffer));

    
    char writeBuffer[65536];
    FileWriteStream os(stdout, writeBuffer, sizeof(writeBuffer));
    Writer<FileWriteStream> writer(os);

    
    CapitalizeFilter<Writer<FileWriteStream> > filter(writer);
    if (!reader.Parse(is, filter)) {
        fprintf(stderr, "\nError(%u): %s\n", (unsigned)reader.GetErrorOffset(), GetParseError_En(reader.GetParseErrorCode()));
        return 1;
    }

    return 0;
}
