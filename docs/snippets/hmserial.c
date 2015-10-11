// Defaults for HMID and HMSR (serial number)
//
// Fallback values are defined below; to override these, use
// compiler switches
// -DEF_HMID=0xFAFBFC
// -DEF_HMSR='\"ABC6543210\"'

#ifndef EF_HMID
#define EF_HMID 0xF1F2F3
#endif
#ifndef EF_HMSR
#define EF_HMSR "ABC0123456"
#endif

unsigned char HMID[3] = { (EF_HMID>>16)&0xFF , (EF_HMID>>8)&0xFF , (EF_HMID>>0)&0xFF };
unsigned char HMSR[11] = EF_HMSR;
