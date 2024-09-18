#ifdef __EMSCRIPTEN__
#error "False positive for emscripten builds  - ignore"
#endif
#include <xmmintrin.h>
int main() { return 0; }
