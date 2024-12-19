 
void engine::
Init(const std::vector<std::string>& args)
{
	//Window.InitVulkanGraphics();
	Window.InitDirectx12Graphics();

	Registry.AddSystem<render_system>();
	Registry.AddSystem<movement_system>();
	Registry.AddSystem<collision_system>(Window.Width, Window.Height);

	vec2 PlayerPosition = vec2(Window.Width / 2, 150);

	entity Paddle = Registry.CreateEntity();
	Paddle.AddTag("Player");
	Paddle.AddComponent<renderable>();
	Paddle.AddComponent<transform>(PlayerPosition - vec2(0, 50));
	Paddle.AddComponent<rectangle>(vec2(100, 20));

	entity Ball = Registry.CreateEntity();
	Ball.AddTag("Ball");
	Ball.AddComponent<renderable>();
	Ball.AddComponent<transform>(PlayerPosition);
	Ball.AddComponent<velocity>();
	Ball.AddComponent<circle>(10);

	s32 NumOfCols = 16;
	s32 NumOfRows = 8;

    s32 LevelWidth  = (Window.Width / 2) - 80;
    s32 LevelHeight = 400;

    s32 BlockWidth  = LevelWidth / NumOfRows;
    s32 BlockHeight = LevelHeight / NumOfCols;

    for (s32 Y = 0; Y < NumOfRows; ++Y)
    {
        for (s32 X = 0; X < NumOfCols; ++X)
        {
            entity Brick = Registry.CreateEntity();
			Brick.AddToGroup("Brick");
            Brick.AddComponent<renderable>();
            Brick.AddComponent<transform>(vec2(100, 50) + vec2(X * BlockWidth, 400 + Y * BlockHeight));
			Brick.AddComponent<rectangle>(vec2(BlockWidth, BlockHeight));
        }
	}

	AssetStore.AddFont("roboto", "../assets/fonts/Roboto-Regular.ttf", MAX_FONT_SIZE);
	font_t RobotoFont = AssetStore.GetFont("roboto");

	// TODO: move this to function for corresponding data
	// NOTE: for drawing, unfortunately, I need both. So, maybe, I will combine into something for better readability
	{
		utils::texture::input_data TextureInputData = {};
		TextureInputData.Format    = image_format::R8G8B8A8_SRGB;
		TextureInputData.Usage     = image_flags::TF_Sampled | image_flags::TF_ColorTexture;
		TextureInputData.Type	   = image_type::Texture2D;
		TextureInputData.MipLevels = 1;
		TextureInputData.Layers    = 1;

		TextureInputData.SamplerInfo.MinFilter = filter::linear;
		TextureInputData.SamplerInfo.MagFilter = filter::linear;
		TextureInputData.SamplerInfo.MipmapMode = mipmap_mode::linear;
		for(u32 Character = 0; Character < 256; ++Character)
		{
			glyph_t& CharacterFont = RobotoFont.Glyphs[Character];
			if(CharacterFont.Memory) TestFont[Character] = Window.Gfx.GpuMemoryHeap->CreateTexture("Roboto font char: " + std::to_string(Character), CharacterFont.Memory, CharacterFont.Width, CharacterFont.Height, 1, TextureInputData);
		}
	}
}

int engine::
Run()
{
	font_t RobotoFont = AssetStore.GetFont("roboto");

	double FrameRateToChoose = 60.0;
    const double TargetFrameRate = 1000.0 / FrameRateToChoose;
    double TimeLast = window::GetTimestamp();

	double SmoothedFPS = FrameRateToChoose;
	const double SmoothingFactor = 0.05; // NOTE: 0 (slow) and 1 (fast)

	while(Window.IsRunning())
	{
		Window.NewFrame();
		Window.EventsDispatcher.Reset();

		window::EventsDispatcher.Subscribe(this, &engine::OnButtonDown);

		auto Result = window::ProcessMessages();
		if(Result) return *Result;

        double FrameTime = TargetFrameRate - (window::GetTimestamp() - TimeLast);
        if (FrameTime > 0 && FrameTime < TargetFrameRate)
        {
            window::SleepFor(TargetFrameRate - FrameTime);
        }
		double FrameDeltaTime = window::GetTimestamp() - TimeLast;
#ifdef CE_DEBUG
		//if (FrameDeltaTime > TargetFrameRate) FrameDeltaTime = TargetFrameRate;
#endif
		TimeLast = window::GetTimestamp();

		Registry.UpdateSystems(FrameDeltaTime);

		double InstantFPS = 1000.0 / FrameDeltaTime;
		SmoothedFPS = SmoothingFactor * InstantFPS + (1.0 - SmoothingFactor) * SmoothedFPS;
		std::string FrameSpeedString = "Frame " + std::to_string(1000.0 / SmoothedFPS) + "ms, " + std::to_string(SmoothedFPS) + "fps";
		if(!Window.IsGfxPaused)
		{
			Registry.GetSystem<render_system>()->Render(Window.Gfx);
			// TODO: Function for text drawing
			{
				u32 PutOffset = 0;
				for(u32 CharIdx = 0; CharIdx < FrameSpeedString.size(); CharIdx++)
				{
					u8 Char = FrameSpeedString.c_str()[CharIdx];
					glyph_t& CharacterFont = RobotoFont.Glyphs[Char];
					if (CharIdx > 0)
					{
						u8 PreviousChar = FrameSpeedString[CharIdx - 1];

						auto KerningIt = RobotoFont.KerningMap.find({PreviousChar, Char});
						if (KerningIt != RobotoFont.KerningMap.end())
						{
							PutOffset -= KerningIt->second / Scale;
						}
					}
					if(CharacterFont.Memory)
					{
						resource_descriptor CharacterTexture = TestFont[Char];

						vec2 StartPoint = vec2(5, Window.Height - FontSize - 5) + vec2(PutOffset + CharacterFont.OffsetX / Scale, CharacterFont.OffsetY / Scale);
						vec2 Dims = vec2(CharacterFont.Width / Scale, CharacterFont.Height / Scale);
						Window.Gfx.PushRectangle(StartPoint, Dims, CharacterTexture);
					}

					PutOffset += CharacterFont.Advance / Scale;
				}
			}
			Window.Gfx.Compile();
			Window.Gfx.Execute();
		}

		Window.EmitEvents();
		window::EventsDispatcher.DispatchEvents();
		Allocator.UpdateAndReset();
	}

	return 0;
}

void engine::
OnButtonDown(key_down_event& Event)
{
	entity PaddleEntity = Registry.GetEntityByTag("Player");
	entity BallEntity = Registry.GetEntityByTag("Ball");

	transform& Trans = PaddleEntity.GetComponent<transform>();
	rectangle Rect = PaddleEntity.GetComponent<rectangle>();

	transform& BallTrans = BallEntity.GetComponent<transform>();
	velocity& BallVel = BallEntity.GetComponent<velocity>();
	circle BallDesc = BallEntity.GetComponent<circle>();

	vec2 Delta(0.0f, 0.0f);
	if(Event.Code == EC_LEFT)
	{
		Delta.x -= 25.0f;
	}
	if(Event.Code == EC_RIGHT)
	{
		Delta.x += 25.0f;
	}
	if(Event.Code == EC_SPACE)
	{
		BallVel.Direction = vec2(0, 1);
		BallVel.Speed = 0.75f;
	}
	
	Trans.Position += Delta;

	float HalfWidth = Rect.Dims.x / 2.0f;
    if (Trans.Position.x - HalfWidth < 0.0f)
    {
        Trans.Position.x = HalfWidth;
    }
    if (Trans.Position.x + HalfWidth > Window.Width)
    {
        Trans.Position.x = Window.Width - HalfWidth;
    }
}
