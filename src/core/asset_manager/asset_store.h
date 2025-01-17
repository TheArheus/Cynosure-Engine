#ifndef ASSET_STORE_H
#define ASSET_STORE_H

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H

struct pair_hash
{
    template <class T1, class T2>
	std::size_t operator()(const std::pair<T1, T2>& pair) const
	{
        return std::hash<T1>()(pair.first) ^ (std::hash<T2>()(pair.second) << 1);
    }
};

struct font_t
{
	texture_t Atlas;
	s32 MaxAscent;
	s32 MaxDescent;
	std::array<glyph_t, 256> Glyphs;
    std::unordered_map<std::pair<u8, u8>, float, pair_hash> KerningMap;
};

typedef std::unordered_map<std::string, texture_t*> texture_storage;
typedef std::unordered_map<std::string, font_t>     font_storage;

class asset_store
{
	FT_Library FreeTypeLibrary;
    texture_storage Textures;
    font_storage Fonts;

public:
    asset_store()
	{
		if (FT_Init_FreeType(&FreeTypeLibrary))
		{
			fprintf(stderr, "Could not initialize FreeType library\n");
		}
	}
    ~asset_store() { ClearAssets(); };

    void ClearAssets();

    void AddTexture(const std::string& AssetID, const std::string& FilePath);
    texture_t* GetTexture(const std::string& AssetID);

    void AddFont(const std::string& AssetID, const std::string& FilePath, s32 FontSize);
    font_t* GetFont(const std::string& AssetID);
};

#endif
