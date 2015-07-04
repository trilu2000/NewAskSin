const uint8_t cnlAddr[] PROGMEM = {
    0x02, 0x05, 0x0a, 0x0b, 0x0c, 0x14, 0x24, 0x25, 0x26, 0x27, // sIdx 0x00, 10 bytes for Channel0/List0
    0x01,                                                       // sIdx 0x0a,  1 byte  for Channel1/List4
}; // 11 byte

EE::s_cnlTbl cnlTbl[] = {
    // cnl, lst, sIdx, sLen, pAddr;
    {  0,  0, 0x00, 10,  0x000f }, // Channel0/List0, 10 bytes at sIdx 0x00, Addr 0x000f
    {  1,  4, 0x0a,  1,  0x0019 }, // Channel1/List4,  1 byte  at sIdx 0x0a, Addr 0x0019
};
