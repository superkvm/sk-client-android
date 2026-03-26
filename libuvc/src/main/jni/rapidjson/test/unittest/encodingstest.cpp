



















#include "unittest.h"
#include "rapidjson/filereadstream.h"
#include "rapidjson/filewritestream.h"
#include "rapidjson/encodedstream.h"
#include "rapidjson/stringbuffer.h"

using namespace rapidjson;




static const unsigned kCodepointRanges[] = {
    0x0000,     0x007F,     
    0x0080,     0x00FF,     
    0x0100,     0x017F,     
    0x0180,     0x024F,     
    0x0250,     0x02AF,     
    0x02B0,     0x02FF,     
    0x0300,     0x036F,     
    0x0370,     0x03FF,     
    0x0400,     0x04FF,     
    0x0500,     0x052F,     
    0x0530,     0x058F,     
    0x0590,     0x05FF,     
    0x0600,     0x06FF,     
    0x0700,     0x074F,     
    0x0750,     0x077F,     
    0x0780,     0x07BF,     
    0x07C0,     0x07FF,     
    0x0800,     0x083F,     
    0x0840,     0x085F,     
    0x0900,     0x097F,     
    0x0980,     0x09FF,     
    0x0A00,     0x0A7F,     
    0x0A80,     0x0AFF,     
    0x0B00,     0x0B7F,     
    0x0B80,     0x0BFF,     
    0x0C00,     0x0C7F,     
    0x0C80,     0x0CFF,     
    0x0D00,     0x0D7F,     
    0x0D80,     0x0DFF,     
    0x0E00,     0x0E7F,     
    0x0E80,     0x0EFF,     
    0x0F00,     0x0FFF,     
    0x1000,     0x109F,     
    0x10A0,     0x10FF,     
    0x1100,     0x11FF,     
    0x1200,     0x137F,     
    0x1380,     0x139F,     
    0x13A0,     0x13FF,     
    0x1400,     0x167F,     
    0x1680,     0x169F,     
    0x16A0,     0x16FF,     
    0x1700,     0x171F,     
    0x1720,     0x173F,     
    0x1740,     0x175F,     
    0x1760,     0x177F,     
    0x1780,     0x17FF,     
    0x1800,     0x18AF,     
    0x18B0,     0x18FF,     
    0x1900,     0x194F,     
    0x1950,     0x197F,     
    0x1980,     0x19DF,     
    0x19E0,     0x19FF,     
    0x1A00,     0x1A1F,     
    0x1A20,     0x1AAF,     
    0x1B00,     0x1B7F,     
    0x1B80,     0x1BBF,     
    0x1BC0,     0x1BFF,     
    0x1C00,     0x1C4F,     
    0x1C50,     0x1C7F,     
    0x1CD0,     0x1CFF,     
    0x1D00,     0x1D7F,     
    0x1D80,     0x1DBF,     
    0x1DC0,     0x1DFF,     
    0x1E00,     0x1EFF,     
    0x1F00,     0x1FFF,     
    0x2000,     0x206F,     
    0x2070,     0x209F,     
    0x20A0,     0x20CF,     
    0x20D0,     0x20FF,     
    0x2100,     0x214F,     
    0x2150,     0x218F,     
    0x2190,     0x21FF,     
    0x2200,     0x22FF,     
    0x2300,     0x23FF,     
    0x2400,     0x243F,     
    0x2440,     0x245F,     
    0x2460,     0x24FF,     
    0x2500,     0x257F,     
    0x2580,     0x259F,     
    0x25A0,     0x25FF,     
    0x2600,     0x26FF,     
    0x2700,     0x27BF,     
    0x27C0,     0x27EF,     
    0x27F0,     0x27FF,     
    0x2800,     0x28FF,     
    0x2900,     0x297F,     
    0x2980,     0x29FF,     
    0x2A00,     0x2AFF,     
    0x2B00,     0x2BFF,     
    0x2C00,     0x2C5F,     
    0x2C60,     0x2C7F,     
    0x2C80,     0x2CFF,     
    0x2D00,     0x2D2F,     
    0x2D30,     0x2D7F,     
    0x2D80,     0x2DDF,     
    0x2DE0,     0x2DFF,     
    0x2E00,     0x2E7F,     
    0x2E80,     0x2EFF,     
    0x2F00,     0x2FDF,     
    0x2FF0,     0x2FFF,     
    0x3000,     0x303F,     
    0x3040,     0x309F,     
    0x30A0,     0x30FF,     
    0x3100,     0x312F,     
    0x3130,     0x318F,     
    0x3190,     0x319F,     
    0x31A0,     0x31BF,     
    0x31C0,     0x31EF,     
    0x31F0,     0x31FF,     
    0x3200,     0x32FF,     
    0x3300,     0x33FF,     
    0x3400,     0x4DBF,     
    0x4DC0,     0x4DFF,     
    0x4E00,     0x9FFF,     
    0xA000,     0xA48F,     
    0xA490,     0xA4CF,     
    0xA4D0,     0xA4FF,     
    0xA500,     0xA63F,     
    0xA640,     0xA69F,     
    0xA6A0,     0xA6FF,     
    0xA700,     0xA71F,     
    0xA720,     0xA7FF,     
    0xA800,     0xA82F,     
    0xA830,     0xA83F,     
    0xA840,     0xA87F,     
    0xA880,     0xA8DF,     
    0xA8E0,     0xA8FF,     
    0xA900,     0xA92F,     
    0xA930,     0xA95F,     
    0xA960,     0xA97F,     
    0xA980,     0xA9DF,     
    0xAA00,     0xAA5F,     
    0xAA60,     0xAA7F,     
    0xAA80,     0xAADF,     
    0xAB00,     0xAB2F,     
    0xABC0,     0xABFF,     
    0xAC00,     0xD7AF,     
    0xD7B0,     0xD7FF,     
    
    
    
    0xE000,     0xF8FF,     
    0xF900,     0xFAFF,     
    0xFB00,     0xFB4F,     
    0xFB50,     0xFDFF,     
    0xFE00,     0xFE0F,     
    0xFE10,     0xFE1F,     
    0xFE20,     0xFE2F,     
    0xFE30,     0xFE4F,     
    0xFE50,     0xFE6F,     
    0xFE70,     0xFEFF,     
    0xFF00,     0xFFEF,     
    0xFFF0,     0xFFFF,     
    0x10000,    0x1007F,    
    0x10080,    0x100FF,    
    0x10100,    0x1013F,    
    0x10140,    0x1018F,    
    0x10190,    0x101CF,    
    0x101D0,    0x101FF,    
    0x10280,    0x1029F,    
    0x102A0,    0x102DF,    
    0x10300,    0x1032F,    
    0x10330,    0x1034F,    
    0x10380,    0x1039F,    
    0x103A0,    0x103DF,    
    0x10400,    0x1044F,    
    0x10450,    0x1047F,    
    0x10480,    0x104AF,    
    0x10800,    0x1083F,    
    0x10840,    0x1085F,    
    0x10900,    0x1091F,    
    0x10920,    0x1093F,    
    0x10A00,    0x10A5F,    
    0x10A60,    0x10A7F,    
    0x10B00,    0x10B3F,    
    0x10B40,    0x10B5F,    
    0x10B60,    0x10B7F,    
    0x10C00,    0x10C4F,    
    0x10E60,    0x10E7F,    
    0x11000,    0x1107F,    
    0x11080,    0x110CF,    
    0x12000,    0x123FF,    
    0x12400,    0x1247F,    
    0x13000,    0x1342F,    
    0x16800,    0x16A3F,    
    0x1B000,    0x1B0FF,    
    0x1D000,    0x1D0FF,    
    0x1D100,    0x1D1FF,    
    0x1D200,    0x1D24F,    
    0x1D300,    0x1D35F,    
    0x1D360,    0x1D37F,    
    0x1D400,    0x1D7FF,    
    0x1F000,    0x1F02F,    
    0x1F030,    0x1F09F,    
    0x1F0A0,    0x1F0FF,    
    0x1F100,    0x1F1FF,    
    0x1F200,    0x1F2FF,    
    0x1F300,    0x1F5FF,    
    0x1F600,    0x1F64F,    
    0x1F680,    0x1F6FF,    
    0x1F700,    0x1F77F,    
    0x20000,    0x2A6DF,    
    0x2A700,    0x2B73F,    
    0x2B740,    0x2B81F,    
    0x2F800,    0x2FA1F,    
    0xE0000,    0xE007F,    
    0xE0100,    0xE01EF,    
    0xF0000,    0xFFFFF,    
    0x100000,   0x10FFFF,   
    0xFFFFFFFF
};




#define UTF8_ACCEPT 0u
#define UTF8_REJECT 12u

static const unsigned char utf8d[] = {
    
    
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,  9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
    7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,  7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
    8,8,2,2,2,2,2,2,2,2,2,2,2,2,2,2,  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
    10,3,3,3,3,3,3,3,3,3,3,3,3,4,3,3, 11,6,6,6,5,8,8,8,8,8,8,8,8,8,8,8,

    
    
    0,12,24,36,60,96,84,12,12,12,48,72, 12,12,12,12,12,12,12,12,12,12,12,12,
    12, 0,12,12,12,12,12, 0,12, 0,12,12, 12,24,12,12,12,12,12,24,12,24,12,12,
    12,12,12,12,12,12,12,24,12,12,12,12, 12,24,12,12,12,12,12,12,12,24,12,12,
    12,12,12,12,12,12,12,36,12,36,12,12, 12,36,12,12,12,12,12,36,12,36,12,12,
    12,36,12,12,12,12,12,12,12,12,12,12, 
};

static unsigned inline decode(unsigned* state, unsigned* codep, unsigned byte) {
    unsigned type = utf8d[byte];

    *codep = (*state != UTF8_ACCEPT) ?
        (byte & 0x3fu) | (*codep << 6) :
    (0xff >> type) & (byte);

    *state = utf8d[256 + *state + type];
    return *state;
}










TEST(EncodingsTest, UTF8) {
    StringBuffer os, os2;
    for (const unsigned* range = kCodepointRanges; *range != 0xFFFFFFFF; range += 2) {
        for (unsigned codepoint = range[0]; codepoint <= range[1]; ++codepoint) {
            os.Clear();
            UTF8<>::Encode(os, codepoint);
            const char* encodedStr = os.GetString();

            
            {
                unsigned decodedCodepoint = 0;
                unsigned state = 0;

                unsigned decodedCount = 0;
                for (const char* s = encodedStr; *s; ++s)
                    if (!decode(&state, &decodedCodepoint, (unsigned char)*s)) {
                        EXPECT_EQ(codepoint, decodedCodepoint);
                        decodedCount++;
                    }

                if (*encodedStr)                
                    EXPECT_EQ(1u, decodedCount);    

                EXPECT_EQ(UTF8_ACCEPT, state);
                if (UTF8_ACCEPT != state)
                    std::cout << std::hex << codepoint << " " << decodedCodepoint << std::endl;
            }

            
            {
                StringStream is(encodedStr);
                unsigned decodedCodepoint;
                bool result = UTF8<>::Decode(is, &decodedCodepoint);
                EXPECT_TRUE(result);
                EXPECT_EQ(codepoint, decodedCodepoint);
                if (!result || codepoint != decodedCodepoint)
                    std::cout << std::hex << codepoint << " " << decodedCodepoint << std::endl;
            }

            
            {
                StringStream is(encodedStr);
                os2.Clear();
                bool result = UTF8<>::Validate(is, os2);
                EXPECT_TRUE(result);
                EXPECT_EQ(0, StrCmp(encodedStr, os2.GetString()));
            }
        }
    }
}

TEST(EncodingsTest, UTF16) {
    GenericStringBuffer<UTF16<> > os, os2;
    GenericStringBuffer<UTF8<> > utf8os;
    for (const unsigned* range = kCodepointRanges; *range != 0xFFFFFFFF; range += 2) {
        for (unsigned codepoint = range[0]; codepoint <= range[1]; ++codepoint) {
            os.Clear();
            UTF16<>::Encode(os, codepoint);
            const UTF16<>::Ch* encodedStr = os.GetString();

            
            if (codepoint != 0) 
            {
                
                utf8os.Clear();
                UTF8<>::Encode(utf8os, codepoint);

                
                unsigned decodedCodepoint = 0;
                unsigned state = 0;
                UTF16<>::Ch buffer[3], *p = &buffer[0];
                for (const char* s = utf8os.GetString(); *s; ++s) {
                    if (!decode(&state, &decodedCodepoint, (unsigned char)*s))
                        break;
                }

                if (codepoint <= 0xFFFF)
                    *p++ = static_cast<UTF16<>::Ch>(decodedCodepoint);
                else {
                    
                    *p++ = static_cast<UTF16<>::Ch>(0xD7C0 + (decodedCodepoint >> 10));
                    *p++ = static_cast<UTF16<>::Ch>(0xDC00 + (decodedCodepoint & 0x3FF));
                }
                *p++ = '\0';

                EXPECT_EQ(0, StrCmp(buffer, encodedStr));
            }

            
            {
                GenericStringStream<UTF16<> > is(encodedStr);
                unsigned decodedCodepoint;
                bool result = UTF16<>::Decode(is, &decodedCodepoint);
                EXPECT_TRUE(result);
                EXPECT_EQ(codepoint, decodedCodepoint);         
                if (!result || codepoint != decodedCodepoint)
                    std::cout << std::hex << codepoint << " " << decodedCodepoint << std::endl;
            }

            
            {
                GenericStringStream<UTF16<> > is(encodedStr);
                os2.Clear();
                bool result = UTF16<>::Validate(is, os2);
                EXPECT_TRUE(result);
                EXPECT_EQ(0, StrCmp(encodedStr, os2.GetString()));
            }
        }
    }
}

TEST(EncodingsTest, UTF32) {
    GenericStringBuffer<UTF32<> > os, os2;
    for (const unsigned* range = kCodepointRanges; *range != 0xFFFFFFFF; range += 2) {
        for (unsigned codepoint = range[0]; codepoint <= range[1]; ++codepoint) {
            os.Clear();
            UTF32<>::Encode(os, codepoint);
            const UTF32<>::Ch* encodedStr = os.GetString();

            
            {
                GenericStringStream<UTF32<> > is(encodedStr);
                unsigned decodedCodepoint;
                bool result = UTF32<>::Decode(is, &decodedCodepoint);
                EXPECT_TRUE(result);
                EXPECT_EQ(codepoint, decodedCodepoint);         
                if (!result || codepoint != decodedCodepoint)
                    std::cout << std::hex << codepoint << " " << decodedCodepoint << std::endl;
            }

            
            {
                GenericStringStream<UTF32<> > is(encodedStr);
                os2.Clear();
                bool result = UTF32<>::Validate(is, os2);
                EXPECT_TRUE(result);
                EXPECT_EQ(0, StrCmp(encodedStr, os2.GetString()));
            }
        }
    }
}
