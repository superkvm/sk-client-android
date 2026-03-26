




#include "rapidjson/reader.h"
#include "rapidjson/writer.h"
#include "rapidjson/filereadstream.h"
#include "rapidjson/filewritestream.h"
#include "rapidjson/error/en.h"

using namespace rapidjson;

int main(int, char*[]) {
    
    Reader reader;
    char readBuffer[65536];
    FileReadStream is(stdin, readBuffer, sizeof(readBuffer));

    
    char writeBuffer[65536];
    FileWriteStream os(stdout, writeBuffer, sizeof(writeBuffer));
    Writer<FileWriteStream> writer(os);

    
    if (!reader.Parse(is, writer)) {
        fprintf(stderr, "\nError(%u): %s\n", (unsigned)reader.GetErrorOffset(), GetParseError_En(reader.GetParseErrorCode()));
        return 1;
    }

    return 0;
}
