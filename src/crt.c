void* memcpy(void* d, const void* s, unsigned long long n) {
    unsigned char* a = (unsigned char*)d;
    const unsigned char* b = (const unsigned char*)s;
    while (n--) *a++ = *b++;
    return d;
}

void* memset(void* d, int c, unsigned long long n) {
    unsigned char* a = (unsigned char*)d;
    while (n--) *a++ = (unsigned char)c;
    return d;
}

void __chkstk(void) {}
