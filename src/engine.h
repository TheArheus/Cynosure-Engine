#pragma once

#include "intrinsics.h"

#include "core/entity_component_system/entity_systems.h"

#include "core/events/events.hpp"
#include "core/asset_manager/asset_store.h"

#include "core/gfx/renderer.h"

#include "core/entity_component_system/components.h"

#include "game_module.hpp"

#include "core/platform/window.hpp"

constexpr double MAX_FONT_SIZE = 96;

struct ce_gui_context
{
	window* Window = nullptr;
	resource_descriptor FontAtlas;
	font_t* Font = nullptr;

	double FontSize;
	double Scale;
};

inline ce_gui_context* GlobalGuiContext = nullptr;

void CreateGuiContext(window* Window)
{
	if(GlobalGuiContext) return;
	GlobalGuiContext = (ce_gui_context*)calloc(1, sizeof(ce_gui_context));
	GlobalGuiContext->Window = Window;
}

void DestroyGuiContext()
{
	if(GlobalGuiContext) free(GlobalGuiContext);
	GlobalGuiContext = nullptr;
}

void SetGuiFont(font_t* Font, double FontSize)
{
	GlobalGuiContext->FontSize = FontSize;
	GlobalGuiContext->Scale    = MAX_FONT_SIZE / FontSize;
	GlobalGuiContext->Font     = Font;

	utils::texture::input_data TextureInputData = {};
	TextureInputData.Format    = image_format::R8G8B8A8_SRGB;
	TextureInputData.Usage     = image_flags::TF_Sampled | image_flags::TF_ColorTexture;
	TextureInputData.Type	   = image_type::Texture2D;
	TextureInputData.MipLevels = 1;
	TextureInputData.Layers    = 1;

	TextureInputData.SamplerInfo.MinFilter = filter::linear;
	TextureInputData.SamplerInfo.MagFilter = filter::linear;
	TextureInputData.SamplerInfo.MipmapMode = mipmap_mode::linear;
	GlobalGuiContext->FontAtlas = GlobalGuiContext->Window->Gfx.GpuMemoryHeap->CreateTexture("FontAtlas", Font->Atlas.Memory, Font->Atlas.Width, Font->Atlas.Height, 1, TextureInputData);
}

vec2 GuiGetLabelDims(const std::string& Str)
{
	double FontSize  = GlobalGuiContext->FontSize;
	double FontScale = GlobalGuiContext->Scale;
	font_t* Font     = GlobalGuiContext->Font;

    double MinX =  1e30f;
    double MinY =  1e30f;
    double MaxX = -1e30f;
    double MaxY = -1e30f;

	double CursorX = 0;
	double CursorY = 0;
	for (u32 CharIdx = 0; CharIdx < Str.size(); CharIdx++)
	{
		u8 Char = Str.c_str()[CharIdx];
		glyph_t& Glyph = Font->Glyphs[Char];
		if (CharIdx > 0)
		{
			u8 PreviousChar = Str[CharIdx - 1];

			auto KerningIt = Font->KerningMap.find({PreviousChar, Char});
			if (KerningIt != Font->KerningMap.end())
			{
				CursorX -= KerningIt->second / FontScale;
			}
		}

        float GlyphLeft   = CursorX + (Glyph.OffsetX / FontScale);
        float GlyphTop    = CursorY + (Glyph.OffsetY / FontScale);
        float GlyphRight  = GlyphLeft + (Glyph.Width  / FontScale);
        float GlyphBottom = GlyphTop  + (Glyph.Height / FontScale);

        if (GlyphLeft   < MinX) MinX = GlyphLeft;
        if (GlyphTop    < MinY) MinY = GlyphTop;
        if (GlyphRight  > MaxX) MaxX = GlyphRight;
        if (GlyphBottom > MaxY) MaxY = GlyphBottom;

		CursorX += Glyph.Advance / FontScale;
	}

    float Width  = MaxX - MinX;
    float Height = MaxY - MinY;

    return vec2(Width, Height);
}

void GuiLabel(const std::string& Str, vec2 Start = vec2(0))
{
	double FontSize  = GlobalGuiContext->FontSize;
	double FontScale = GlobalGuiContext->Scale;
	font_t* Font     = GlobalGuiContext->Font;

	Start = Start + vec2(5, -(FontSize + 5));
	u32 PutOffset = 0;
	for(u32 CharIdx = 0; CharIdx < Str.size(); CharIdx++)
	{
		u8 Char = Str.c_str()[CharIdx];
		glyph_t& CharacterFont = Font->Glyphs[Char];
		if (CharIdx > 0)
		{
			u8 PreviousChar = Str[CharIdx - 1];

			auto KerningIt = Font->KerningMap.find({PreviousChar, Char});
			if (KerningIt != Font->KerningMap.end())
			{
				PutOffset -= KerningIt->second / FontScale;
			}
		}
		if(CharacterFont.Width != 0 || CharacterFont.Height != 0)
		{
			vec2 StartPoint = Start + vec2(PutOffset + CharacterFont.OffsetX / FontScale, CharacterFont.OffsetY / FontScale);
			vec2 OffsetPoint = vec2(CharacterFont.StartX, CharacterFont.StartY);
			vec2 Scale = vec2(CharacterFont.Width / FontScale, CharacterFont.Height / FontScale);
			vec2 Dims = vec2(CharacterFont.Width, CharacterFont.Height);
			GlobalGuiContext->Window->Gfx.PushRectangle(StartPoint, Scale, OffsetPoint, Dims, GlobalGuiContext->FontAtlas);
		}

		PutOffset += CharacterFont.Advance / FontScale;
	}
}

bool GuiButton(const std::string& Label, vec2 Pos, vec2 Size)
{
	vec2 MousePos = vec2(GlobalGuiContext->Window->MouseX, GlobalGuiContext->Window->MouseY);
	bool LeftDown = GlobalGuiContext->Window->Buttons[EC_LBUTTON].IsDown;

	float Left   = Pos.x - Size.x * 0.5f;
	float Bottom = Pos.y - Size.y * 0.5f;
	float Right  = Pos.x + Size.x * 0.5f;
	float Top    = Pos.y + Size.y * 0.5f;

    bool IsHovered = (MousePos.x >= Left && MousePos.x <= Right &&
                      MousePos.y >= Bottom && MousePos.y <= Top);

    bool IsClicked = false;
    if (IsHovered && LeftDown)
    {
        IsClicked = true;
    }

    vec3 Color = vec3(0.25f, 0.25f, 0.25f);
    if (IsHovered) Color = vec3(0.4f, 0.4f, 0.4f);
    if (IsClicked) Color = vec3(0.3f, 0.6f, 0.3f);

	vec2 LabelDims = GuiGetLabelDims(Label);
	float TextPosX = Left + (Size.x - LabelDims.x) * 0.5f;
	float TextPosY = Top  - (Size.y - LabelDims.y) * 0.5f;

	GlobalGuiContext->Window->Gfx.PushRectangle(Pos, Size, Color);
	GuiLabel(Label, vec2(TextPosX, TextPosY));

	return IsClicked;
}

enum class game_state
{
    main_menu,
    playing,
    paused
};

class engine
{
	window Window;
	registry Registry;
	asset_store AssetStore;

	game_module_create NewGameModule;

	void DrawMainMenu();
	void DrawPauseMenu();
	void SetupGameEntities();
	void OnButtonDown(key_down_event& Event);

	game_state CurrentGameState = game_state::main_menu;

public:
	engine(const std::vector<std::string>& args) : Window("3D Renderer") { Init(args); };

	void Init(const std::vector<std::string>& args);
	int Run();
};

