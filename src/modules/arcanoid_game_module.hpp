
class arcanoid : game_module
{
public:
	void OnInit()
	{
		Registry.AddSystem<render_system>();
		Registry.AddSystem<movement_system>();
		Registry.AddSystem<collision_system>();

		entity Paddle = Registry.CreateEntity();
		Paddle.AddComponent<transform>(vec2(300, 50), vec2(100, 20));
		Paddle.AddComponent<paddle>(500.0f);

		entity Ball = Registry.CreateEntity();
		Ball.AddComponent<transform>(vec2(300, 100), vec2(10, 10));
		Ball.AddComponent<velocity>(vec2(1e-10f, 1e-10f), 300.0f);
		Ball.AddComponent<ball>();

		for (int x = 0; x < 10; ++x)
		{
			for (int y = 0; y < 5; ++y)
			{
				entity Brick = Registry.CreateEntity();
				Brick.AddComponent<transform>(vec2(x * 50, 400 + y * 30), vec2(50, 20));
				Brick.AddComponent<brick>(1);
			}
		}
	}

	void OnUpdate(double dt)
	{
		Registry.UpdateSystems();
		Registry.GetSystem<movement_system>()->Update(dt);
		Registry.GetSystem<collision_system>()->Update(dt);
	}

	void OnRender()
	{
		Registry.GetSystem<render_system>()->Render(Window.Gfx);
	}
};
