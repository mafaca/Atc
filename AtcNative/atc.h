#ifndef ATC_H
#define ATC_H

#include <cstdint>

void DecodeAtcRgb4(const uint8_t* data, const int w, const int h, uint32_t* image);
void DecodeAtcRgba8(const uint8_t* data, const int w, const int h, uint32_t* image);

#endif /* end of include guard: ATC_H */