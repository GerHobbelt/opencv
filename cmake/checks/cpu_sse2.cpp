#ifdef __EMSCRIPTEN__
#error "False positive for emscripten builds  - ignore"
#endif
#include <emmintrin.h>
int main() { return 0; }
