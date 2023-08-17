#ifndef b64encode
#define b64encode

char *b64_encode(const unsigned char *in, size_t len);
size_t b64_encoded_size(size_t inlen);

#endif
