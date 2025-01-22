#pragma once

// TODO: make so that it would work with as little dependencies as possible

constexpr double MAX_FONT_SIZE = 145;

struct ce_gui_context
{
	window* Window = nullptr;
	resource_descriptor FontAtlas;
	font_t* Font = nullptr;

	std::vector<prim_vert> Vertices;
	std::vector<u32> Indices;

	u64 PrimitiveCount = 0;
	double FontSize;
	double Scale;
};

inline ce_gui_context* GlobalGuiContext = nullptr;

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
		float Kerning = 0.0;
		if (CharIdx > 0)
		{
			u8 PreviousChar = Str[CharIdx - 1];

			auto KerningIt = Font->KerningMap.find({PreviousChar, Char});
			if (KerningIt != Font->KerningMap.end())
			{
				Kerning = KerningIt->second;
			}
		}

        if (Char == ' ')
        {
            CursorX += FontScale;
            continue;
        }
        if (Char == '\t')
        {
            CursorX += 4 * FontScale;
            continue;
        }
        if (Char == '\n')
        {
            CursorX = 0;
            CursorY -= FontScale;
            continue;
        }

        float GlyphTop    = CursorY  + (Glyph.OffsetY / float(FontScale));
        float GlyphBottom = GlyphTop - (Glyph.Height / float(FontScale));
        float GlyphLeft   = CursorX   + (Glyph.OffsetX / float(FontScale));
        float GlyphRight  = GlyphLeft + (Glyph.Width  / float(FontScale));

        if (GlyphTop    > MaxY) MaxY = GlyphTop;
        if (GlyphBottom < MinY) MinY = GlyphBottom;
        if (GlyphLeft   < MinX) MinX = GlyphLeft;
        if (GlyphRight  > MaxX) MaxX = GlyphRight;

		CursorX += (Glyph.AdvanceX + Kerning) / FontScale;
	}

    float Width  = MaxX - MinX;
    float Height = MaxY - MinY;

    return vec2(Width, Height);
}

void GuiLabel(const std::string& Str, vec2 BaselineStart = vec2(0))
{
	double FontSize  = GlobalGuiContext->FontSize;
	double FontScale = GlobalGuiContext->Scale;
	font_t* Font     = GlobalGuiContext->Font;

	vec2 AtlasDims = vec2(GlobalGuiContext->FontAtlas.Width, GlobalGuiContext->FontAtlas.Height);

	BaselineStart = BaselineStart + vec2(0.75f * FontScale, -(Font->MaxAscent + Font->MaxDescent) / float(FontScale));
	float PutOffsetX = 0;
	float PutOffsetY = 0;
	for(u32 CharIdx = 0; CharIdx < Str.size(); CharIdx++)
	{
		u8 Char = Str.c_str()[CharIdx];
		glyph_t& CharacterFont = Font->Glyphs[Char];
		float Kerning = 0.0f;
		if (CharIdx > 0)
		{
			u8 PreviousChar = Str[CharIdx - 1];

			auto KerningIt = Font->KerningMap.find({PreviousChar, Char});
			if (KerningIt != Font->KerningMap.end())
			{
				Kerning = KerningIt->second;
			}
		}

        if (Char == ' ')
        {
            PutOffsetX += FontScale;
            continue;
        }
        if (Char == '\t')
        {
            PutOffsetX += 4 * FontScale;
            continue;
        }
        if (Char == '\n')
        {
            PutOffsetX = 0;
            PutOffsetY -= FontScale;
            continue;
        }

		if(CharacterFont.Width != 0 || CharacterFont.Height != 0)
		{
			vec2 StartPoint = BaselineStart + vec2(PutOffsetX + CharacterFont.OffsetX / float(FontScale), PutOffsetY + CharacterFont.OffsetY / float(FontScale));
			vec2 OffsetPoint = vec2(CharacterFont.StartX, CharacterFont.StartY);
			vec2 Scale = vec2(CharacterFont.Width / float(FontScale), float(CharacterFont.Height) / float(FontScale));
			vec2 Dims = vec2(CharacterFont.Width, CharacterFont.Height);

			PushRectangle(GlobalGuiContext->Vertices, GlobalGuiContext->Indices, StartPoint, Scale, OffsetPoint, Dims, AtlasDims);
		}

		PutOffsetX += (CharacterFont.AdvanceX + Kerning) / FontScale;
	}
}

bool GuiButton(const std::string& Label, vec2 Pos, vec2 Size)
{
	vec2 MousePos = vec2(GlobalGuiContext->Window->MouseX, GlobalGuiContext->Window->MouseY);
	bool LeftDown = GlobalGuiContext->Window->Buttons[EC_LBUTTON].IsDown && !GlobalGuiContext->Window->Buttons[EC_LBUTTON].WasDown;

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

	PushRectangle(GlobalGuiContext->Vertices, GlobalGuiContext->Indices, Pos, Size, Color);
	//GlobalGuiContext->Window->Gfx.PushRectangle(Pos, Size, Color);
	GuiLabel(Label, vec2(TextPosX, TextPosY));

	return IsClicked;
}

bool GuiCheckBox(const std::string& Label, bool& Value, vec2 Pos)
{
    float BoxSize = GlobalGuiContext->Scale;

    float Left   = Pos.x;
    float Bottom = Pos.y - BoxSize;
    float Right  = Pos.x + BoxSize;
    float Top    = Pos.y;

    vec2 MousePos = vec2(GlobalGuiContext->Window->MouseX, GlobalGuiContext->Window->MouseY);
    bool LeftDown = GlobalGuiContext->Window->Buttons[EC_LBUTTON].IsDown && 
                    !GlobalGuiContext->Window->Buttons[EC_LBUTTON].WasDown;

    bool IsHovered = (MousePos.x >= Left && MousePos.x <= Right &&
                      MousePos.y >= Bottom && MousePos.y <= Top);

    bool IsClicked = false;
    if (IsHovered && LeftDown)
    {
        IsClicked = true;
        Value = !Value;
    }

    vec3 BoxColor = vec3(0.3f, 0.3f, 0.3f);
    if (IsHovered) BoxColor = vec3(0.5f, 0.5f, 0.5f);

    vec2 BoxPos = vec2((Left + Right)*0.5f, (Bottom + Top)*0.5f);
    vec2 BoxDims = vec2(BoxSize, BoxSize);
	PushRectangle(GlobalGuiContext->Vertices, GlobalGuiContext->Indices, BoxPos, BoxDims, BoxColor);

    if (Value)
    {
        vec3 CheckColor = vec3(0.1f, 0.8f, 0.1f);
		PushRectangle(GlobalGuiContext->Vertices, GlobalGuiContext->Indices, BoxPos, BoxDims * 0.6, CheckColor);
    }

    vec2 LabelPos = vec2(Right + 5.0f, Top);
    GuiLabel(Label, LabelPos);

    return IsClicked;
}

bool GuiRadioButton(const std::string& Label, int& CurrentValue, int ThisValue, vec2 Pos)
{
	double FontScale = GlobalGuiContext->Scale;
    float Radius = FontScale;

    float Left   = Pos.x - Radius;
    float Bottom = Pos.y - Radius;
    float Right  = Pos.x + Radius;
    float Top    = Pos.y + Radius;

    vec2 MousePos = vec2(GlobalGuiContext->Window->MouseX, GlobalGuiContext->Window->MouseY);
    bool LeftDown = GlobalGuiContext->Window->Buttons[EC_LBUTTON].IsDown && 
                    !GlobalGuiContext->Window->Buttons[EC_LBUTTON].WasDown;

    bool IsHovered = (MousePos.x >= Left && MousePos.x <= Right &&
                      MousePos.y >= Bottom && MousePos.y <= Top);

    bool IsClicked = false;
    if (IsHovered && LeftDown)
    {
        IsClicked = true;
        CurrentValue = ThisValue;
    }

    vec3 CircleColor = vec3(0.3f, 0.3f, 0.3f);
    if (IsHovered) CircleColor = vec3(0.5f, 0.5f, 0.5f);

    vec2 CirclePos  = Pos;
	PushCircle(GlobalGuiContext->Vertices, GlobalGuiContext->Indices, CirclePos, Radius, CircleColor);

    if (CurrentValue == ThisValue)
    {
        vec3 InnerColor = vec3(0.1f, 0.8f, 0.1f);
		PushCircle(GlobalGuiContext->Vertices, GlobalGuiContext->Indices, CirclePos, Radius * 0.65f, InnerColor);
    }

    vec2 LabelPos = vec2(Pos.x + Radius + 5, Pos.y + Radius);
    GuiLabel(Label, LabelPos);

    return IsClicked;
}

bool GuiSlider(const std::string& Label, float& Value, float MinValue, float MaxValue, vec2 Pos, float Width)
{
    float TrackHeight = 10.0f;
    float HalfWidth = Width * 0.5f;

    float t = (Value - MinValue) / (MaxValue - MinValue);
    if (t < 0.0f) t = 0.0f; 
    if (t > 1.0f) t = 1.0f;

    float HandleRadius = 10.0f;
    float HandleX = Pos.x - HalfWidth + t * Width;
    float HandleY = Pos.y;

    float SliderLeft   = Pos.x - HalfWidth - HandleRadius;
    float SliderRight  = Pos.x + HalfWidth + HandleRadius;
    float SliderBottom = Pos.y - HandleRadius;
    float SliderTop    = Pos.y + HandleRadius;

    vec2 MousePos = vec2(GlobalGuiContext->Window->MouseX, GlobalGuiContext->Window->MouseY);
    bool LeftPressed   = GlobalGuiContext->Window->Buttons[EC_LBUTTON].IsDown;
    bool LeftClicked   = LeftPressed && !GlobalGuiContext->Window->Buttons[EC_LBUTTON].WasDown;
    static bool IsDragging = false;

    bool IsHovered = (MousePos.x >= SliderLeft && MousePos.x <= SliderRight &&
                      MousePos.y >= SliderBottom && MousePos.y <= SliderTop);

    if (IsHovered && LeftClicked)
    {
        IsDragging = true;
    }
    if (!LeftPressed)
    {
        IsDragging = false;
    }

    bool ValueChanged = false;
    if (IsDragging)
    {
        float NewT = (MousePos.x - (Pos.x - HalfWidth)) / Width;
        if (NewT < 0.0f) NewT = 0.0f;
        if (NewT > 1.0f) NewT = 1.0f;

        float NewValue = MinValue + NewT * (MaxValue - MinValue);
        if (fabs(NewValue - Value) > 0.0001f)
        {
            Value = NewValue;
            ValueChanged = true;
        }

        t = NewT;
        HandleX = Pos.x - HalfWidth + t * Width;
    }

    vec3 TrackColor = vec3(0.3f, 0.3f, 0.3f);
    vec2 TrackPos   = Pos; 
    vec2 TrackSize  = vec2(Width, TrackHeight);
	PushRectangle(GlobalGuiContext->Vertices, GlobalGuiContext->Indices, TrackPos, TrackSize, TrackColor);

    vec3 HandleColor = IsDragging ? vec3(0.3f, 0.6f, 0.3f) : vec3(0.6f, 0.6f, 0.6f);
	PushRectangle(GlobalGuiContext->Vertices, GlobalGuiContext->Indices, vec2(HandleX, HandleY), vec2(HandleRadius*2), HandleColor);

    GuiLabel(Label, vec2(Pos.x - HalfWidth, Pos.y + 20.0f));
    GuiLabel(std::to_string(Value), vec2(Pos.x + HalfWidth + 10, Pos.y + 20.0f));

    return ValueChanged;
}
