#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <cstdlib>
#include <cstdio>

void asset_store::
ClearAssets()
{
    for(auto Texture : Textures)
    {
        free(Texture.second->Memory);
        free(Texture.second);
    }

    for(auto& Font : Fonts)
    {
		for(u32 Idx = 0; Idx < 256; Idx++)
		{
			if(Font.second.Glyphs[Idx].Memory) free(Font.second.Glyphs[Idx].Memory);
		}
    }

    Textures.clear();
    Fonts.clear();
	FT_Done_FreeType(FreeTypeLibrary);
}

void asset_store::AddTexture(const std::string& AssetID, const std::string& FilePath)
{
    s32 Width, Height, Pitch;
    unsigned char* Data = stbi_load(FilePath.c_str(), &Width, &Height, &Pitch, 0);

    texture_t* Result = (texture_t*)malloc(sizeof(texture_t));
    Result->Width  = Width;
    Result->Height = Height;
    Result->Memory = (u32*)malloc(Width * Height * Pitch);

    for (s32 y = 0; y < Height; ++y)
    {
        memcpy(
            (u8*)Result->Memory + y * Width * Pitch,
            Data + (Height - 1 - y) * Width * Pitch,
            Width * Pitch
        );
    }

    stbi_image_free(Data);
    Textures[AssetID] = Result;
}

texture_t* asset_store::
GetTexture(const std::string& AssetID)
{
	auto it = Textures.find(AssetID);
	if(it != Textures.end())
	{
		return it->second;
	}
	return nullptr;
}

void asset_store::
AddFont(const std::string& AssetID, const std::string& FilePath, s32 FontSize)
{
    FT_Face FontFace;

    if (FT_New_Face(FreeTypeLibrary, FilePath.c_str(), 0, &FontFace))
    {
        fprintf(stderr, "Failed to load font: %s\n", FilePath.c_str());
        return;
    }

    FT_Set_Pixel_Sizes(FontFace, 0, FontSize);

	font_t Font;
    for (u32 Character = 0; Character < 256; ++Character)
    {
		FT_Load_Char(FontFace, Character, FT_LOAD_RENDER);
        FT_GlyphSlot Glyph = FontFace->glyph;

        glyph_t& GlyphData = Font.Glyphs[Character];
        GlyphData.Width = Glyph->bitmap.width;
        GlyphData.Height = Glyph->bitmap.rows;
        GlyphData.OffsetX = Glyph->bitmap_left;
        GlyphData.OffsetY = Glyph->bitmap_top;
        GlyphData.Advance = Glyph->advance.x >> 6;

        if (Glyph->bitmap.width == 0 || Glyph->bitmap.rows == 0)
        {
			GlyphData.Memory = nullptr;
            continue;
        }

        u32 BitmapSize = GlyphData.Width * GlyphData.Height;
        GlyphData.Memory = (u32*)malloc(BitmapSize * sizeof(u32));

        for (u32 Y = 0; Y < GlyphData.Height; ++Y)
        {
            for (u32 X = 0; X < GlyphData.Width; ++X)
            {
                u8 Gray = Glyph->bitmap.buffer[(GlyphData.Height - 1 - Y) * GlyphData.Width + X];
                GlyphData.Memory[Y * GlyphData.Width + X] = (Gray << 24) | (Gray << 16) | (Gray << 8) | Gray;
            }
        }
    }

    if (FT_HAS_KERNING(FontFace))
    {
        for (u32 First = 0; First < 256; ++First)
        {
            for (u32 Second = 0; Second < 256; ++Second)
            {
                FT_Vector Kerning;
                FT_Get_Kerning(FontFace, FT_Get_Char_Index(FontFace, First), FT_Get_Char_Index(FontFace, Second), FT_KERNING_DEFAULT, &Kerning);

                if (Kerning.x != 0)
                {
                    Font.KerningMap[{(u8)First, (u8)Second}] = Kerning.x >> 6;
                }
            }
        }
    }

    Fonts[AssetID] = std::move(Font);
    FT_Done_Face(FontFace);
}

font_t asset_store::
GetFont(const std::string& AssetID)
{
	auto it = Fonts.find(AssetID);
	if(it != Fonts.end())
	{
		return it->second;
	}
	return {};
}
