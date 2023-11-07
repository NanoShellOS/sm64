//#include <PR/os.h>
#include <string.h>

void bcopy(const void* src, void* dst, size_t sz) {
	memcpy(dst, src, sz);
}
int bcmp(const void* s1, const void* s2, size_t sz) {
	int result = memcmp(s1, s2, sz);
	if (result < 0) return -1;
	if (result > 0) return 1;
	return result;
}
void bzero(void* mem, size_t sz) {
	memset(mem, 0, sz);
}