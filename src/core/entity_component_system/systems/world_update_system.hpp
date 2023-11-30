#pragma once

struct world_update_system : entity_system
{
	bool IsCameraLocked = false;
	bool IsDebugColors  = false;

	mat4 LockedCameraProj;
	mat4 LockedCameraView;

	system_constructor(world_update_system)
	{
		RequireComponent<camera_component>();
	}

	void Update(window& Window, global_world_data& WorldUpdate, mesh_comp_culling_common_input& MeshCompCullingCommonData)
	{
		camera_component* CurrentCameraData = Entities[0].GetComponent<camera_component>();

		float FOV   = CurrentCameraData->ProjectionData.FOV;
		float NearZ = CurrentCameraData->ProjectionData.NearZ;
		float FarZ  = CurrentCameraData->ProjectionData.FarZ;
		mat4  CameraProj = PerspRH(FOV, Window.Gfx->Width, Window.Gfx->Height, NearZ, FarZ);
		mat4  CameraView = LookAtRH(vec3(0, 0, 0), vec3(0, 0, 1), vec3(0, 1, 0));
		LockedCameraProj = CameraProj;
		LockedCameraView = CameraView;

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
		//WorldUpdate.CameraPos		= vec4(ViewPos, 1);
		//WorldUpdate.CameraDir		= vec4(ViewDir, 0);
		//WorldUpdate.GlobalLightPos	= vec4(LightPos, 1);
		WorldUpdate.GlobalLightSize = 1;
		WorldUpdate.NearZ			= NearZ;
		WorldUpdate.FarZ			= FarZ;
		WorldUpdate.DebugColors		= IsDebugColors;

		// TODO: UI
		MeshCompCullingCommonData.FrustrumCullingEnabled  = false;
		MeshCompCullingCommonData.OcclusionCullingEnabled = false;
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

			vec3 FrustumCenter(0);
			for(vec3 V : CameraViewCorners)
			{
				FrustumCenter += V;
			}
			FrustumCenter /= 8.0;

			float Radius = -INFINITY;
			for(vec3 V : CameraViewCorners)
			{
				float Dist = Length(V - FrustumCenter);
				Radius = Max(Radius, Dist);
			}
			WorldUpdate.LightView[CascadeIdx - 1];// = LookAtRH(FrustumCenter - LightDir * Radius, FrustumCenter, vec3(0, 1, 0));
			WorldUpdate.LightProj[CascadeIdx - 1];// = OrthoRH(-Radius, Radius, Radius, -Radius, 0.0f, 2.0f * Radius);

			vec4 ShadowOrigin = vec4(0, 0, 0, 1);
			mat4 ShadowMatrix = WorldUpdate.LightView[CascadeIdx - 1] * WorldUpdate.LightProj[CascadeIdx - 1];
			ShadowOrigin = ShadowMatrix * ShadowOrigin;
			ShadowOrigin;// = ShadowOrigin * GlobalShadow[CascadeIdx - 1].Width / 2.0f;

			vec4 RoundedOrigin = vec4(round(ShadowOrigin.x), round(ShadowOrigin.y), round(ShadowOrigin.z), round(ShadowOrigin.w));
			vec4 RoundOffset = RoundedOrigin - ShadowOrigin;
			RoundOffset;// = RoundOffset * 2.0f / GlobalShadow[CascadeIdx - 1].Width;
			RoundOffset.z = 0.0f;
			RoundOffset.w = 0.0f;
			WorldUpdate.LightProj[CascadeIdx - 1].Line3 += RoundOffset;
		}
	}
};
