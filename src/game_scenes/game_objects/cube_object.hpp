
// TODO: Maybe achieve that every object would get different behavior in every scene???
struct cube_object : object_behavior
{
	~cube_object() override {}

	void Start() override
	{
		Mesh.Load("..\\assets\\cube.obj", generate_aabb | generate_sphere);
	}

	void Update() override
	{
		u32 SceneRadius = 10;
		for(u32 DataIdx = 0;
			DataIdx < 512;
			DataIdx++)
		{
			vec4 Translate = vec4((float(rand()) / RAND_MAX) * 2 * SceneRadius - SceneRadius, 
								  (float(rand()) / RAND_MAX) * 2 * SceneRadius - SceneRadius, 
								  (float(rand()) / RAND_MAX) * 2 * SceneRadius - SceneRadius, 0.0f);

			vec4 Scale = vec4(vec3(1.0f / 2.0), 1.0);
			AddInstance({vec4(1, 1, 1, 1), 0, 0, 0, 0}, Translate, Scale, true);
		}
	}
};

