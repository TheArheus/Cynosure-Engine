#include "arcanoid_game_module.h"

#include "core/asset_manager/asset_store.cpp"
#include "core/entity_component_system/entity_systems.cpp"
#include "systems.hpp"

void arcanoid::
ModuleStart()
{
	Registry.AddSystem<render_system>(Gfx);
	Registry.AddSystem<movement_system>();
	Registry.AddSystem<collision_system>(Window.Width, Window.Height);

	EventsDispatcher.Subscribe(this, &arcanoid::OnButtonDown);
	EventsDispatcher.Subscribe(this, &arcanoid::OnButtonUp);
	EventsDispatcher.Subscribe(this, &arcanoid::OnButtonHold);
}

void arcanoid::
ModuleUpdate()
{
	switch(CurrentGameState)
	{
		case game_state::main_menu:
		{
			DrawMainMenu();
		} break;

		case game_state::playing:
		{
			Registry.UpdateSystems(dt);
			Registry.GetSystem<render_system>()->Render(Gfx, vec2(Window.Width, Window.Height));
		} break;

		case game_state::paused:
		{
			DrawPauseMenu();
		} break;
	}
}

void arcanoid::
DrawMainMenu()
{
#if USE_CE_GUI
    vec2 ScreenCenter = vec2(Window.Width * 0.5f, Window.Height * 0.5f);

    vec2 BtnSize = vec2(200, 50);
    vec2 BtnPos  = ScreenCenter + vec2(0, 30);
    if (GuiButton("Start Game", BtnPos, BtnSize))
    {
        CurrentGameState = game_state::playing;

        SetupGameEntities();
    }

    BtnPos = ScreenCenter - vec2(0, 30);
    if (GuiButton("Exit Game", BtnPos, BtnSize))
    {
		Window.RequestClose();
    }
#else
    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(Window.Width, Window.Height), ImGuiCond_Always);
    
    ImGuiWindowFlags WindowFlags  = ImGuiWindowFlags_NoTitleBar |
                                    ImGuiWindowFlags_NoResize |
                                    ImGuiWindowFlags_NoMove |
                                    ImGuiWindowFlags_NoScrollbar |
                                    ImGuiWindowFlags_NoScrollWithMouse |
                                    ImGuiWindowFlags_NoCollapse |
                                    ImGuiWindowFlags_NoBackground;

    ImGui::Begin("MainMenu", nullptr, WindowFlags);

    vec2 ScreenCenter = vec2(Window.Width * 0.5f, Window.Height * 0.5f);
    vec2 BtnSize = vec2(200, 50);

    vec2 BtnPosStart = vec2(ScreenCenter.x, ScreenCenter.y - 30);
    ImGui::SetCursorPos(BtnPosStart - 0.5f * BtnSize);
    if (ImGui::Button("Start Game", BtnSize))
    {
        CurrentGameState = game_state::playing;
        SetupGameEntities();
    }

    ImVec2 BtnPosExit = ImVec2(ScreenCenter.x, ScreenCenter.y + 30);
    ImGui::SetCursorPos(BtnPosExit - 0.5f * BtnSize);
    if (ImGui::Button("Exit Game", BtnSize))
    {
        Window.RequestClose();
    }

    ImGui::End();
#endif
}

void arcanoid::
DrawPauseMenu()
{
#if USE_CE_GUI
    vec2 ScreenCenter = vec2(Window.Width * 0.5f, Window.Height * 0.5f);

    vec2 BtnSize = vec2(200, 50);
    vec2 BtnPos  = ScreenCenter + vec2(0, 30);
    if (GuiButton("Resume", BtnPos, BtnSize))
    {
        CurrentGameState = game_state::playing;
    }

    BtnPos = ScreenCenter - vec2(0, 30);
    if (GuiButton("Main Menu", BtnPos, BtnSize))
    {
        Registry.ClearAllEntities();
        CurrentGameState = game_state::main_menu;
    }
#else
    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(Window.Width, Window.Height), ImGuiCond_Always);
    
    ImGuiWindowFlags WindowFlags  = ImGuiWindowFlags_NoTitleBar |
                                    ImGuiWindowFlags_NoResize |
                                    ImGuiWindowFlags_NoMove |
                                    ImGuiWindowFlags_NoScrollbar |
                                    ImGuiWindowFlags_NoScrollWithMouse |
                                    ImGuiWindowFlags_NoCollapse |
                                    ImGuiWindowFlags_NoBackground;

    ImGui::Begin("PauseMenu", nullptr, WindowFlags);

    vec2 ScreenCenter = vec2(Window.Width * 0.5f, Window.Height * 0.5f);
    vec2 BtnSize = vec2(200, 50);

    vec2 BtnPosStart = vec2(ScreenCenter.x, ScreenCenter.y - 30);
    ImGui::SetCursorPos(BtnPosStart - 0.5f * BtnSize);
    if (ImGui::Button("Resume", BtnSize))
    {
        CurrentGameState = game_state::playing;
    }

    ImVec2 BtnPosExit = ImVec2(ScreenCenter.x, ScreenCenter.y + 30);
    ImGui::SetCursorPos(BtnPosExit - 0.5f * BtnSize);
    if (ImGui::Button("Main Menu", BtnSize))
    {
        Registry.ClearAllEntities();
        CurrentGameState = game_state::main_menu;
    }

    ImGui::End();
#endif
}

void arcanoid::
SetupGameEntities()
{
	Registry.ClearAllEntities();

	vec2 PlayerPosition = vec2(Window.Width / 2, 150);

	entity Paddle = Registry.CreateEntity();
	Paddle.AddTag("Player");
	Paddle.AddComponent<renderable>();
	Paddle.AddComponent<collidable>();
	Paddle.AddComponent<transform>(PlayerPosition - vec2(0, 50));
	Paddle.AddComponent<rectangle>(vec2(100, 20));

	entity Ball = Registry.CreateEntity();
	Ball.AddTag("Ball");
	Ball.AddComponent<renderable>();
	Ball.AddComponent<collidable>();
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
			Brick.AddComponent<collidable>();
			Brick.AddComponent<transform>(vec2(100, 50) + vec2(X * BlockWidth, 400 + Y * BlockHeight));
			Brick.AddComponent<rectangle>(vec2(BlockWidth, BlockHeight));
		}
	}
}

void arcanoid::
OnButtonUp(key_up_event& Event)
{
	if(CurrentGameState == game_state::playing)
	{
		entity BallEntity = Registry.GetEntityByTag("Ball");
		velocity& BallVel = BallEntity.GetComponent<velocity>();

		if(Event.Code == EC_SPACE)
		{
			BallVel.Direction = vec2(0, 1);
			BallVel.Speed = 0.75f;
		}
	}
}

void arcanoid::
OnButtonDown(key_down_event& Event)
{
    if (CurrentGameState == game_state::playing)
    {
        if (Event.Code == EC_ESCAPE)
        {
            CurrentGameState = game_state::paused;
        }
    }
	else if (CurrentGameState == game_state::paused)
    {
        if (Event.Code == EC_ESCAPE)
        {
            CurrentGameState = game_state::playing;
        }
    }
}

void arcanoid::
OnButtonHold(key_hold_event& Event)
{
	if(CurrentGameState == game_state::playing)
	{
		entity PaddleEntity = Registry.GetEntityByTag("Player");

		transform& Trans = PaddleEntity.GetComponent<transform>();
		rectangle Rect = PaddleEntity.GetComponent<rectangle>();

		vec2 Delta(0.0f, 0.0f);
		if(Event.Code == EC_LEFT)
		{
			Delta.x -= 25.0f;
		}
		if(Event.Code == EC_RIGHT)
		{
			Delta.x += 25.0f;
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
}

extern "C" GameModuleCreateFunc(GameModuleCreate)
{
	Allocator = NewAllocator;
	ImGui::SetCurrentContext(NewWindow.imguiContext.get());
	GlobalGuiContext = NewContext;
	game_module* Ptr = new arcanoid(NewWindow, NewEventDispatcher, NewRegistry, NewGfx);
	return Ptr;
}
