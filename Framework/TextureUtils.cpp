
#include "TextureUtils.h"
#include "dds.h"

#include <cassert>
#include <cstdio>
#include <string>
#include <algorithm>

#ifdef _MSC_VER
#	include <Windows.h>
#	include <Shlobj.h>
#endif

GLint map_Format_Internal[] = {
	0,
	GL_RGB8,
	GL_RGB8,
	GL_RGBA8,
	GL_SRGB8_ALPHA8,

	GL_COMPRESSED_RGBA_S3TC_DXT1_EXT,
	GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT,
	GL_COMPRESSED_RGBA_S3TC_DXT5_EXT,
	GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT,

	GL_RG16F,
	GL_RGBA16F,
	GL_RG32F
};

GLenum map_Format_Format[] = {
	0,
	GL_RGB,
	GL_BGR,
	GL_RGBA,
	GL_RGBA,

	GL_RGBA,
	GL_RGBA,
	GL_RGBA,
	GL_RGBA,

	GL_RG,
	GL_RGBA,
	GL_RG
};

GLenum map_Format_Type[] = {
	0,
	GL_UNSIGNED_BYTE,
	GL_UNSIGNED_BYTE,
	GL_UNSIGNED_BYTE,
	GL_UNSIGNED_BYTE,

	GL_UNSIGNED_BYTE,
	GL_UNSIGNED_BYTE,
	GL_UNSIGNED_BYTE,
	GL_UNSIGNED_BYTE,

	GL_HALF_FLOAT,
	GL_HALF_FLOAT,
	GL_FLOAT
};

static uint32_t NextPow2(uint32_t x)
{
	--x;

	x |= x >> 1;
	x |= x >> 2;
	x |= x >> 4;
	x |= x >> 8;
	x |= x >> 16;

	return ++x;
}

CTextureUtils::CTextureUtils()
{
}

CTextureUtils::~CTextureUtils()
{
}

GLuint CTextureUtils::FindAndLoadTexture(const wchar_t* filename, bool srgb)
{
#ifndef _MSC_VER
	// TODO: platform specific code for Linux/macOS
	assert(false);
#endif

	// NOTE: defined in property sheet
	std::wstring texturefile(MY_MEDIA_DIR);
	std::wstring ext;
	GLuint texid = 0;

	if (FALSE == PathResolve(&texturefile[0], NULL, PRF_VERIFYEXISTS)) {
		printf("[TextureUtils] Media directory not found\n");
		return 0;
	}

	texturefile.resize(texturefile.find_first_of(L'\0'));
	texturefile += L"\\Textures\\";
	texturefile += filename;

	ext = texturefile.substr(texturefile.find_last_of(L'.') + 1);

	if (ext == L"dds") {
		DDS_Image_Info info;

		if (!LoadFromDDS(texturefile.c_str(), &info)) {
			printf("[TextureUtils] Could not load texture '%S'\n", texturefile.c_str());
			return 0;
		}

		glGenTextures(1, &texid);

		if (info.Type == TextureType2D) {
			glBindTexture(GL_TEXTURE_2D, texid);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			if (info.MipLevels > 1)
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			else
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

			if (info.Format == PixelFormatCompressedDXT1 || info.Format == PixelFormatCompressedDXT5) {
				// TODO:
				assert(false);
			} else {
				// uncompressed
				uint32_t bytes = 4;

				if (info.Format == PixelFormatRG32Float)
					bytes = 8;
				else if (info.Format == PixelFormatRG16Float)
					bytes = 4;
				else if (info.Format == PixelFormatRGB8Unorm || info.Format == PixelFormatBGR8Unorm) {
					bytes = 3;
				}

				if (srgb) {
					// TODO: change format
					assert(false);
				}

				uint32_t mipsize = info.Width * info.Height * bytes;

				glTexImage2D(GL_TEXTURE_2D, 0, map_Format_Internal[info.Format], info.Width, info.Height, 0,
					map_Format_Format[info.Format], map_Format_Type[info.Format], info.Data);

				if (info.MipLevels > 1)
					glGenerateMipmap(GL_TEXTURE_2D);
			}
		} else if (info.Type == TextureTypeCube) {
			glBindTexture(GL_TEXTURE_CUBE_MAP, texid);

			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			if (info.MipLevels > 1)
				glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			else
				glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

			GLsizei pow2s = NextPow2(info.Width);
			GLsizei facesize;
			
			if (info.Format == PixelFormatCompressedDXT1 || info.Format == PixelFormatCompressedDXT5) {
				// TODO:
				assert(false);
			} else {
				// uncompressed
				GLsizei size;
				GLsizei offset = 0;
				GLsizei bytes = 4;

				if (info.Format == PixelFormatARGB16Float)
					bytes = 8;
				else
					assert(false);

				for (int i = 0; i < 6; ++i) {
					for (uint32_t j = 0; j < info.MipLevels; ++j) {
						size = std::max<uint32_t>(1, pow2s >> j);
						facesize = size * size * bytes;

						glTexImage2D(
							GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, j, map_Format_Internal[info.Format], size, size, 0,
							map_Format_Format[info.Format], map_Format_Type[info.Format], (char*)info.Data + offset);

						offset += facesize;
					}
				}
			}
		} else {
			// TODO:
			assert(false);
		}

		if (info.Data)
			free(info.Data);
	} else {
		// TODO:
		assert(false);
	}

	GLenum err = glGetError();

	if (err != GL_NO_ERROR) {
		glDeleteTextures(1, &texid);
		texid = 0;

		printf("[TextureUtils] OpenGL error 0x%x\n", err);
	}

	return texid;
}
