
#ifndef _DDS_H_
#define _DDS_H_

// NOTE: adapted from Quadron

#include "TextureUtils.h"

struct DDS_Image_Info
{
	uint32_t	Width;
	uint32_t	Height;
	uint32_t	Depth;
	PixelFormat	Format;
	TextureType	Type;
	uint32_t	MipLevels;
	uint32_t	DataSize;
	void*		Data;
};

bool LoadFromDDS(const wchar_t* file, DDS_Image_Info* outinfo);

uint32_t GetImageSize(uint32_t width, uint32_t height, uint32_t bytes, uint32_t miplevels);
uint32_t GetCompressedImageSize(uint32_t width, uint32_t height, uint32_t miplevels, uint32_t format);
uint32_t GetCompressedImageSize(uint32_t width, uint32_t height, uint32_t depth, uint32_t miplevels, uint32_t format);
uint32_t GetCompressedLevelSize(uint32_t width, uint32_t height, uint32_t level, uint32_t format);
uint32_t GetCompressedLevelSize(uint32_t width, uint32_t height, uint32_t depth, uint32_t level, uint32_t format);

#endif
