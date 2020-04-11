#include "atc.h"
#include <algorithm>
#include <cstring>

static inline uint16_t uload16(const void* p)
{
	return *(uint16_t*)p;
}

static inline uint32_t uload32(const void* p)
{
	return *(uint32_t*)p;
}

static inline uint64_t uload64(const void* p)
{
	return *(uint64_t*)p;
}

constexpr uint32_t u32_extend(uint32_t value, int from, int to)
{
	// bit-pattern replicating scaling (can at most double the bits)
	return (value << (to - from)) | (value >> (from * 2 - to));
}

inline void DecodeColors(uint8_t* colors, uint32_t c0, uint32_t c1)
{
	if (c0 & 0x8000)
	{
		colors[0] = 0;
		colors[1] = 0;
		colors[2] = 0;

		colors[8] = u32_extend((c0 >> 0) & 0x1f, 5, 8);
		colors[9] = u32_extend((c0 >> 5) & 0x1f, 5, 8);
		colors[10] = u32_extend((c0 >> 10) & 0x1f, 5, 8);

		colors[12] = u32_extend((c1 >> 0) & 0x1f, 5, 8);
		colors[13] = u32_extend((c1 >> 5) & 0x3f, 6, 8);
		colors[14] = u32_extend((c1 >> 11) & 0x1f, 5, 8);

		colors[4] = std::max(0, colors[8] - colors[12] / 4);
		colors[5] = std::max(0, colors[9] - colors[13] / 4);
		colors[6] = std::max(0, colors[10] - colors[14] / 4);
	}
	else
	{
		colors[0] = u32_extend((c0 >> 0) & 0x1f, 5, 8);
		colors[1] = u32_extend((c0 >> 5) & 0x1f, 5, 8);
		colors[2] = u32_extend((c0 >> 10) & 0x1f, 5, 8);

		colors[12] = u32_extend((c1 >> 0) & 0x1f, 5, 8);
		colors[13] = u32_extend((c1 >> 5) & 0x3f, 6, 8);
		colors[14] = u32_extend((c1 >> 11) & 0x1f, 5, 8);

		colors[4] = (5 * colors[0] + 3 * colors[12]) / 8;
		colors[5] = (5 * colors[1] + 3 * colors[13]) / 8;
		colors[6] = (5 * colors[2] + 3 * colors[14]) / 8;

		colors[8] = (3 * colors[0] + 5 * colors[12]) / 8;
		colors[9] = (3 * colors[1] + 5 * colors[13]) / 8;
		colors[10] = (3 * colors[2] + 5 * colors[14]) / 8;
	}
}

inline void DecodeAlphas(uint8_t* alphas, uint8_t a0, uint8_t a1)
{
	alphas[0] = a0;
	alphas[1] = a1;
	if (a0 > a1)
	{
		alphas[2] = (a0 * 6 + a1 * 1) / 7;
		alphas[3] = (a0 * 5 + a1 * 2) / 7;
		alphas[4] = (a0 * 4 + a1 * 3) / 7;
		alphas[5] = (a0 * 3 + a1 * 4) / 7;
		alphas[6] = (a0 * 2 + a1 * 5) / 7;
		alphas[7] = (a0 * 1 + a1 * 6) / 7;
	}
	else
	{
		alphas[2] = (a0 * 4 + a1 * 1) / 5;
		alphas[3] = (a0 * 3 + a1 * 2) / 5;
		alphas[4] = (a0 * 2 + a1 * 3) / 5;
		alphas[5] = (a0 * 1 + a1 * 4) / 5;
		alphas[6] = 0;
		alphas[7] = 255;
	}
}

void DecodeAtcRgb4Block(const uint8_t* data, uint8_t* outbuf)
{
	uint32_t c0 = uload16(data + 0);
	uint32_t c1 = uload16(data + 2);
	uint32_t indices = uload32(data + 4);

	uint8_t colors[16];
	DecodeColors(colors, c0, c1);

	for (int y = 0; y < 4; ++y)
	{
		for (int x = 0; x < 4; ++x)
		{
			int idx = indices & 3;
			indices >>= 2;
			outbuf[y * 4 * 4 + x * 4 + 0] = colors[idx * 4 + 0];
			outbuf[y * 4 * 4 + x * 4 + 1] = colors[idx * 4 + 1];
			outbuf[y * 4 * 4 + x * 4 + 2] = colors[idx * 4 + 2];
			outbuf[y * 4 * 4 + x * 4 + 3] = 0xff;
		}
	}
}

void DecodeAtcRgba8Block(const uint8_t* data, uint8_t* outbuf)
{
	uint8_t alphas[16];
	uint64_t avalue = uload64(data);
	uint8_t a0 = (avalue >> 0) & 0xFF;
	uint8_t a1 = (avalue >> 8) & 0xFF;
	uint64_t aindex = avalue >> 16;

	uint8_t colors[16];
	uint32_t c0 = uload16(data + 8);
	uint32_t c1 = uload16(data + 10);
	uint32_t cindex = uload32(data + 12);

	DecodeColors(colors, c0, c1);
	DecodeAlphas(alphas, a0, a1);


	for (int y = 0; y < 4; ++y)
	{
		for (int x = 0; x < 4; ++x)
		{
			int cidx = cindex & 3;
			int aidx = aindex & 7;
			cindex >>= 2;
			aindex >>= 3;
			outbuf[y * 4 * 4 + x * 4 + 0] = colors[cidx * 4 + 0];
			outbuf[y * 4 * 4 + x * 4 + 1] = colors[cidx * 4 + 1];
			outbuf[y * 4 * 4 + x * 4 + 2] = colors[cidx * 4 + 2];
			outbuf[y * 4 * 4 + x * 4 + 3] = alphas[aidx];
		}
	}
}

void DecodeAtcRgb4(const uint8_t* data, const int w, const int h, uint32_t* image)
{
	int bcw = (w + 3) / 4;
	int bch = (h + 3) / 4;
	int clen_last = (w + 3) % 4 + 1;
	uint32_t buf[16];
	const uint8_t* d = (uint8_t*)data;
	for (int t = 0; t < bch; t++) {
		for (int s = 0; s < bcw; s++, d += 8) {
			DecodeAtcRgb4Block(d, (uint8_t*)buf);
			int clen = (s < bcw - 1 ? 4 : clen_last) * 4;
			for (int i = 0, y = h - t * 4 - 1; i < 4 && y >= 0; i++, y--)
				memcpy(image + y * w + s * 4, buf + i * 4, clen);
		}
	}
}

void DecodeAtcRgba8(const uint8_t* data, const int w, const int h, uint32_t* image)
{
	int bcw = (w + 3) / 4;
	int bch = (h + 3) / 4;
	int clen_last = (w + 3) % 4 + 1;
	uint32_t buf[16];
	const uint8_t* d = (uint8_t*)data;
	for (int t = 0; t < bch; t++) {
		for (int s = 0; s < bcw; s++, d += 16) {
			DecodeAtcRgba8Block(d, (uint8_t*)buf);
			int clen = (s < bcw - 1 ? 4 : clen_last) * 4;
			for (int i = 0, y = h - t * 4 - 1; i < 4 && y >= 0; i++, y--)
				memcpy(image + y * w + s * 4, buf + i * 4, clen);
		}
	}
}