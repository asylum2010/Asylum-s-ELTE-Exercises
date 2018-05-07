
#ifndef _TEXTUREUTILS_H_
#define _TEXTUREUTILS_H_

#include <GL/glew.h>
#include <cstdint>

enum PixelFormat
{
	PixelFormatInvalid = 0,
	PixelFormatRGB8Unorm,
	PixelFormatBGR8Unorm,
	PixelFormatARGB8Unorm,
	PixelFormatARGB8Unorm_sRGB,
	PixelFormatCompressedDXT1,
	PixelFormatCompressedDXT1_sRGB,
	PixelFormatCompressedDXT5,
	PixelFormatCompressedDXT5_sRGB,
	PixelFormatRG16Float,
	PixelFormatARGB16Float,
	PixelFormatRG32Float
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
	static void Shutdown();
};

#endif
