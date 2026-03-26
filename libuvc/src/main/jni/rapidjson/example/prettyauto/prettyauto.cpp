



#include "rapidjson/reader.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/filereadstream.h"
#include "rapidjson/filewritestream.h"
#include "rapidjson/encodedstream.h"    
#include "rapidjson/error/en.h"
#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif

using namespace rapidjson;

int main(int, char*[]) {
#ifdef _WIN32
    
    _setmode(_fileno(stdin), _O_BINARY);    
    _setmode(_fileno(stdout), _O_BINARY);   
#endif

    
    
    GenericReader<AutoUTF<unsigned>, UTF8<> > reader;       
    char readBuffer[65536];
    FileReadStream is(stdin, readBuffer, sizeof(readBuffer));
    AutoUTFInputStream<unsigned, FileReadStream> eis(is);   

    
    char writeBuffer[65536];
    FileWriteStream os(stdout, writeBuffer, sizeof(writeBuffer));

#if 1
    
    typedef AutoUTFOutputStream<unsigned, FileWriteStream> OutputStream;    
    OutputStream eos(os, eis.GetType(), eis.HasBOM());                      
    PrettyWriter<OutputStream, UTF8<>, AutoUTF<unsigned> > writer(eos);     
#else
    
    typedef EncodedOutputStream<UTF16LE<>,FileWriteStream> OutputStream;    
    OutputStream eos(os, true);                                             
    PrettyWriter<OutputStream, UTF8<>, UTF16LE<> > writer(eos);             
#endif

    
    
    if (!reader.Parse<kParseValidateEncodingFlag>(eis, writer)) {   
        fprintf(stderr, "\nError(%u): %s\n", (unsigned)reader.GetErrorOffset(), GetParseError_En(reader.GetParseErrorCode()));
        return 1;
    }

    return 0;
}
