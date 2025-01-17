#pragma once

struct world_update_system : entity_system
{
	vec3 ViewPos;
	vec3 ViewDir;
	vec3 LockedViewPos;
	vec3 LockedViewDir;
	vec3 FixedSceneCenter;

	bool IsCameraLocked = false;
	bool IsDebugColors  = false;

	world_update_system()
	{
		RequireComponent<camera_component>();

		ViewPos = vec3(0);
		ViewDir = vec3(1, 0, 0);
		FixedSceneCenter = ViewPos;
	}

	void SubscribeToEvents(event_bus& Events)
	{
		Events.Subscribe(this, &world_update_system::OnButtonDown);
	}

	void Update(window& Window, global_world_data& WorldUpdate, mesh_comp_culling_common_input& MeshCompCullingCommonData, vec3 GlobalLightPos)
	{
		camera_component& CurrentCameraData = Entities[0].GetComponent<camera_component>();
		vec3 GlobalLightDir = -Normalize(GlobalLightPos);
		u32  GlobalShadowWidth  = PreviousPowerOfTwo(Window.Gfx.Backend->Width) * 2;
		u32  GlobalShadowHeight = PreviousPowerOfTwo(Window.Gfx.Backend->Width) * 2;

		if(!IsCameraLocked)
		{
			LockedViewPos = ViewPos;
			LockedViewDir = ViewDir;
		}

		float FOV   = CurrentCameraData.ProjectionData.FOV;
		float NearZ = CurrentCameraData.ProjectionData.NearZ;
		float FarZ  = CurrentCameraData.ProjectionData.FarZ;
		mat4  CameraProj = PerspRH(FOV, Window.Gfx.Backend->Width, Window.Gfx.Backend->Height, NearZ, FarZ);
		mat4  CameraView = LookAtRH(ViewPos, ViewPos + ViewDir, vec3(0, 1, 0));
		mat4  LockedCameraView = LookAtRH(LockedViewPos, LockedViewPos + LockedViewDir, vec3(0, 1, 0));

		WorldUpdate.CascadeSplits[0] = NearZ;
		for(u32 CascadeIdx = 1;
			CascadeIdx < DEPTH_CASCADES_COUNT;
			++CascadeIdx)
		{
			WorldUpdate.CascadeSplits[CascadeIdx] = 
										Lerp(NearZ * powf(FarZ / NearZ, float(CascadeIdx) / DEPTH_CASCADES_COUNT),
											 0.75,
											 NearZ + (float(CascadeIdx) / DEPTH_CASCADES_COUNT) * (FarZ - NearZ));
		}
		WorldUpdate.CascadeSplits[DEPTH_CASCADES_COUNT] = FarZ;

		WorldUpdate.View			= CameraView;
		WorldUpdate.DebugView		= LockedCameraView;
		WorldUpdate.Proj			= CameraProj;
		WorldUpdate.CameraPos		= vec4(ViewPos, 1);
		WorldUpdate.CameraDir		= vec4(ViewDir, 0);
		WorldUpdate.GlobalLightPos	= vec4(GlobalLightPos, 1);
		WorldUpdate.ScreenWidth		= Window.Gfx.Backend->Width;
		WorldUpdate.ScreenHeight	= Window.Gfx.Backend->Height;
		WorldUpdate.NearZ			= NearZ;
		WorldUpdate.FarZ			= FarZ;
		WorldUpdate.DebugColors		= IsDebugColors;

		WorldUpdate.SceneScale  = vec4(vec3(1.0 / (VOXEL_SIZE / 8.0)), 0.0);
#if 0
		WorldUpdate.SceneCenter = vec4(
				vec3(floorf(WorldUpdate.CameraPos.x / VOXEL_SIZE) * VOXEL_SIZE,
					 floorf(WorldUpdate.CameraPos.y / VOXEL_SIZE) * VOXEL_SIZE,
					 floorf(WorldUpdate.CameraPos.z / VOXEL_SIZE) * VOXEL_SIZE), 
				0.0);
#else
		if(Distance(WorldUpdate.CameraPos.xyz, FixedSceneCenter) > 1.0 / 8.0)
		{
			FixedSceneCenter = WorldUpdate.CameraPos.xyz;
		}
		WorldUpdate.SceneCenter = vec4(FixedSceneCenter, 0.0);
#endif

		// TODO: UI
		MeshCompCullingCommonData.NearZ = NearZ;

		MeshCompCullingCommonData.Proj = CameraProj;
		MeshCompCullingCommonData.View = WorldUpdate.DebugView;
		GeneratePlanes(MeshCompCullingCommonData.CullingPlanes, CameraProj, 1);

		std::vector<u32> FrustrumIndices = 
		{
			0, 1, 1, 2, 2, 3, 3, 0,
			4, 5, 5, 6, 6, 7, 7, 4,
			0, 4, 1, 5, 2, 6, 3, 7,
		};

		// TODO: try to move ortho projection calculation to compute shader???
		// TODO: move into game_main everything related to cascade splits calculation
		for(u32 CascadeIdx = 1;
			CascadeIdx <= DEPTH_CASCADES_COUNT;
			++CascadeIdx)
		{
			std::vector<vertex> DebugFrustum;

			float LightNearZ = WorldUpdate.CascadeSplits[CascadeIdx - 1];
			float LightFarZ  = WorldUpdate.CascadeSplits[CascadeIdx];
			float LightDistZ = LightFarZ - LightNearZ;

			mat4 CameraCascadeProj = PerspRH(FOV, WorldUpdate.ScreenWidth, WorldUpdate.ScreenHeight, LightNearZ, LightFarZ);
			mat4 InverseProjectViewMatrix = Inverse(LockedCameraView * CameraCascadeProj);

			std::vector<vec4> CameraViewCorners = 
			{
				// Near
				vec4{-1.0f, -1.0f, 0.0f, 1.0f},
				vec4{ 1.0f, -1.0f, 0.0f, 1.0f},
				vec4{ 1.0f,  1.0f, 0.0f, 1.0f},
				vec4{-1.0f,  1.0f, 0.0f, 1.0f},
				// Far
				vec4{-1.0f, -1.0f, 1.0f, 1.0f},
				vec4{ 1.0f, -1.0f, 1.0f, 1.0f},
				vec4{ 1.0f,  1.0f, 1.0f, 1.0f},
				vec4{-1.0f,  1.0f, 1.0f, 1.0f},
			};
			std::vector<vec4> LightAABBCorners = CameraViewCorners; // NOTE: Light View Corners
			std::vector<vec4> LightViewCorners = CameraViewCorners; // NOTE: Light View Corners

			for(size_t i = 0; i < CameraViewCorners.size(); i++)
			{
				CameraViewCorners[i] = InverseProjectViewMatrix * CameraViewCorners[i];
				CameraViewCorners[i] = CameraViewCorners[i] / CameraViewCorners[i].w;

				DebugFrustum.push_back({CameraViewCorners[i], {}, 0});
			}
			// TODO: Dynamic meshes and dynamic instances
			//DebugMeshInstances.push_back({vec4(0), vec4(1), DebugGeometries.Load(DebugFrustum, FrustrumIndices)});
			//DebugMeshVisibility.push_back(true);

			vec3 FrustumCenter(0.0f);
			for(vec3 V : CameraViewCorners)
			{
				FrustumCenter += V;
			}
			FrustumCenter /= 8.0;

			float Radius = 0.0f;
			for(vec3 V : CameraViewCorners)
			{
				float Dist = Length(V - FrustumCenter);
				Radius = Max(Radius, Dist);
			}
			WorldUpdate.LightView[CascadeIdx - 1] = LookAtRH(FrustumCenter - GlobalLightDir * Radius, FrustumCenter, vec3(0, 1, 0));
			WorldUpdate.LightProj[CascadeIdx - 1] = OrthoRH(-Radius, Radius, Radius, -Radius, 0.0f, 2.0f * Radius);

			vec4 ShadowOrigin = vec4(0, 0, 0, 1);
			mat4 ShadowMatrix = WorldUpdate.LightView[CascadeIdx - 1] * WorldUpdate.LightProj[CascadeIdx - 1];
			ShadowOrigin = ShadowMatrix * ShadowOrigin;
			ShadowOrigin = ShadowOrigin * GlobalShadowWidth / 2.0f;

			vec4 RoundedOrigin = vec4(round(ShadowOrigin.x), round(ShadowOrigin.y), round(ShadowOrigin.z), round(ShadowOrigin.w));
			vec4 RoundOffset = RoundedOrigin - ShadowOrigin;
			RoundOffset = RoundOffset * 2.0f / GlobalShadowWidth;
			RoundOffset.z = 0.0f;
			RoundOffset.w = 0.0f;
			WorldUpdate.LightProj[CascadeIdx - 1].E41 += RoundOffset.x;
			WorldUpdate.LightProj[CascadeIdx - 1].E42 += RoundOffset.y;
			WorldUpdate.LightProj[CascadeIdx - 1].E43 += RoundOffset.z;
			WorldUpdate.LightProj[CascadeIdx - 1].E44 += RoundOffset.w;
		}
	}

	void OnButtonDown(key_hold_event& Event)
	{
		if(Event.Code == EC_I)
		{
			IsDebugColors = !IsDebugColors;
		}
		if(Event.Code == EC_L)
		{
			IsCameraLocked = !IsCameraLocked;
		}

		float CameraSpeed = 0.01f;
		if(Event.Code == EC_R)
		{
			ViewPos += vec3(0, 4.0f*CameraSpeed, 0);
		}
		if(Event.Code == EC_F)
		{
			ViewPos -= vec3(0, 4.0f*CameraSpeed, 0);
		}
		if(Event.Code == EC_W)
		{
			ViewPos += (ViewDir * 4.0f*CameraSpeed);
		}
		if(Event.Code == EC_S)
		{
			ViewPos -= (ViewDir * 4.0f*CameraSpeed);
		}
#if 0
		vec3 z = (ViewData.ViewDir - ViewData.CameraPos).Normalize();
		vec3 x = Cross(vec3(0, 1, 0), z).Normalize();
		vec3 y = Cross(z, x);
		vec3 Horizontal = x;
		if(GameInput.Buttons[EC_D].IsDown)
		{
			ViewData.CameraPos -= Horizontal * CameraSpeed;
		}
		if(GameInput.Buttons[EC_A].IsDown)
		{
			ViewData.CameraPos += Horizontal * CameraSpeed;
		}
#endif
		if(Event.Code == EC_LEFT)
		{
			quat ViewDirQuat(ViewDir, 0);
			quat RotQuat( CameraSpeed * 2, vec3(0, 1, 0));
			ViewDir = (RotQuat * ViewDirQuat * RotQuat.Inverse()).v.xyz;
		}
		if(Event.Code == EC_RIGHT)
		{
			quat ViewDirQuat(ViewDir, 0);
			quat RotQuat(-CameraSpeed * 2, vec3(0, 1, 0));
			ViewDir = (RotQuat * ViewDirQuat * RotQuat.Inverse()).v.xyz;
		}
		vec3 U = Cross(vec3(0, 1, 0), ViewDir);
		if(Event.Code == EC_UP)
		{
			quat ViewDirQuat(ViewDir, 0);
			quat RotQuat( CameraSpeed, U);
			ViewDir = (RotQuat * ViewDirQuat * RotQuat.Inverse()).v.xyz;
		}
		if(Event.Code == EC_DOWN)
		{
			quat ViewDirQuat(ViewDir, 0);
			quat RotQuat(-CameraSpeed, U);
			ViewDir = (RotQuat * ViewDirQuat * RotQuat.Inverse()).v.xyz;
		}
	}
};
