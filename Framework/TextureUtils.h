
#ifndef _TEXTUREUTILS_H_
#define _TEXTUREUTILS_H_

#include <GL/glew.h>
#include <cstdint>

enum PixelFormat
{
	PixelFormatFormatInvalid = 0,
	PixelFormatFormatRGB8Unorm,
	PixelFormatFormatBGR8Unorm,
	PixelFormatFormatARGB8Unorm,
	PixelFormatFormatARGB8Unorm_sRGB,
	PixelFormatFormatCompressedDXT1,
	PixelFormatFormatCompressedDXT1_sRGB,
	PixelFormatFormatCompressedDXT5,
	PixelFormatFormatCompressedDXT5_sRGB,
	PixelFormatFormatRG16Float,
	PixelFormatFormatARGB16Float,
	PixelFormatFormatRG32Float
};

enum TextureType
{
	TextureType2D = 0,
	TextureTypeCube,
	TextureTypeVolume,
};

class CTextureUtils
{
private:
	CTextureUtils();
	~CTextureUtils();

public:
	static GLuint FindAndLoadTexture(const wchar_t* filename, bool srgb);
};

#endif
