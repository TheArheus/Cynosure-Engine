#pragma once

// TODO: generating the actual rendering commands here
// TODO: managing instances
struct render_system : public entity_system
{
	render_system()
	{
		RequireComponent<renderable>();
	}

	void Render(global_graphics_context& Gfx)
	{
		for(entity& Entity : Entities)
		{
			auto& Transform = Entity.GetComponent<transform>();
			if(Entity.HasComponent<mesh>())
			{
			}
			if(Entity.HasComponent<circle>())
			{
				auto& CircleInfo = Entity.GetComponent<circle>();
				Gfx.PushCircle(Transform.Position, Transform.Scale.x * CircleInfo.Radius, CircleInfo.Color);
			}
			if(Entity.HasComponent<rectangle>())
			{
				auto& RectInfo = Entity.GetComponent<rectangle>();
				Gfx.PushRectangle(Transform.Position, Transform.Scale * RectInfo.Dims, RectInfo.Color);
			}
			if(Entity.HasComponent<rectangle_textured>())
			{
				auto& RectInfo = Entity.GetComponent<rectangle_textured>();
				Gfx.PushRectangle(Transform.Position, Transform.Scale * RectInfo.Dims, RectInfo.Texture);
			}
		}
	}
};

struct movement_system : public entity_system
{
	movement_system()
	{
		RequireComponent<transform>();
		RequireComponent<velocity>();
	}

	void Update(double dt) override
	{
		for(entity& Entity : Entities)
		{
            auto& Transform = Entity.GetComponent<transform>();
            auto& Velocity  = Entity.GetComponent<velocity>();

            Transform.Position += Velocity.Direction * Velocity.Speed * dt;
		}
	}
};

static float Clamp(float Value, float MinVal, float MaxVal)
{
    return Max(MinVal, Min(Value, MaxVal));
}

static bool CircleRectCollision(const vec2& CirclePos, float Radius, const vec2& RectPos, const vec2& RectSize, vec2& Normal, float& Depth)
{
    float HalfWidth  = RectSize.x * 0.5f;
    float HalfHeight = RectSize.y * 0.5f;

    float Left   = RectPos.x - HalfWidth;
    float Right  = RectPos.x + HalfWidth;
    float Top    = RectPos.y - HalfHeight;
    float Bottom = RectPos.y + HalfHeight;

    float ClosestX = Clamp(CirclePos.x, Left, Right);
    float ClosestY = Clamp(CirclePos.y, Top, Bottom);

    float DistanceX = CirclePos.x - ClosestX;
    float DistanceY = CirclePos.y - ClosestY;

    float DistanceSquared = DistanceX * DistanceX + DistanceY * DistanceY;
    if (DistanceSquared > Radius * Radius)
        return false;

    float Distance = std::sqrt(DistanceSquared);
    if (Distance != 0.0f)
	{
        Normal = vec2(DistanceX / Distance, DistanceY / Distance);
        Depth  = Radius - Distance;
    }
    else
	{
        float OverlapLeft   = CirclePos.x - Left;
        float OverlapRight  = Right - CirclePos.x;
        float OverlapTop    = CirclePos.y - Top;
        float OverlapBottom = Bottom - CirclePos.y;

        float MinOverlap = OverlapLeft;
        Normal = vec2{ -1.0f, 0.0f };

        if (OverlapRight < MinOverlap)
		{
            MinOverlap = OverlapRight;
            Normal = vec2{ 1.0f, 0.0f };
        }
        if (OverlapTop < MinOverlap)
		{
            MinOverlap = OverlapTop;
            Normal = vec2{ 0.0f, -1.0f };
        }
        if (OverlapBottom < MinOverlap)
		{
            MinOverlap = OverlapBottom;
            Normal = vec2{ 0.0f, 1.0f };
        }

        Depth = Radius + MinOverlap;
    }

    return true;
}

struct collision_system : public entity_system
{
	u32 FrameWidth;
	u32 FrameHeight;
	collision_system(u32 WindowWidth, u32 WindowHeight) : FrameWidth(WindowWidth), FrameHeight(WindowHeight) {}

	void SubscribeOnEvents() override
	{
		window::EventsDispatcher.Subscribe(this, &collision_system::OnCollision);
	}

	void OnCollision(collision_event& Event)
	{
		circle& BallInfo = Event.A.GetComponent<circle>();
        velocity& BallVel = Event.A.GetComponent<velocity>();
        transform& BallTrans = Event.A.GetComponent<transform>();

        if (Event.A.HasTag("Ball") && Event.B.BelongsToGroup("Brick"))
		{
            transform& BrickTrans = Event.B.GetComponent<transform>();

			BallTrans.Position += Event.Normal * Event.Depth;

			BallVel.Direction = BallVel.Direction - 2.0f * Dot(BallVel.Direction, Event.Normal) * Event.Normal;
			BallVel.Direction = BallVel.Direction.Normalize();

            Event.B.Kill();
        }
        else if (Event.A.HasTag("Ball") && Event.B.HasTag("Player"))
		{
            transform& PaddleTrans = Event.B.GetComponent<transform>();
			rectangle& PaddleInfo  = Event.B.GetComponent<rectangle>();

			BallTrans.Position += Event.Normal * Event.Depth;

            float HitPos = (BallTrans.Position.x - PaddleTrans.Position.x) / (0.5f * PaddleInfo.Dims.x);
            HitPos = Clamp(HitPos, -1.0f, 1.0f);

            float Angle = HitPos * (75.0f * (3.14159f / 180.0f)); // 75 degrees max angle
            BallVel.Direction = vec2(std::sin(Angle), std::cos(Angle)).Normalize();
        }
	}

	void Update(double dt) override
	{
		for (entity BallEntity : Entities)
		{
			if(!BallEntity.HasTag("Ball")) continue;
			transform& BallTrans = BallEntity.GetComponent<transform>();
			velocity& BallVel = BallEntity.GetComponent<velocity>();
			circle BallDesc = BallEntity.GetComponent<circle>();

			if (BallTrans.Position.y + BallDesc.Radius >= FrameHeight)
			{
				BallTrans.Position.y = FrameHeight - BallDesc.Radius;
				BallVel.Direction.y *= -1;
			}
			if (BallTrans.Position.y - BallDesc.Radius <= 0.0f)
			{
				BallTrans.Position = vec2(FrameWidth * 0.5f, 150.0f);
				BallVel.Direction = vec2(0.0f, 0.0f);
				BallVel.Speed = 0.0f;
			}
			if (BallTrans.Position.x - BallDesc.Radius <= 0.0f)
			{
				BallTrans.Position.x = BallDesc.Radius;
				BallVel.Direction.x *= -1;
			}
			if (BallTrans.Position.x + BallDesc.Radius >= FrameWidth)
			{
				BallTrans.Position.x = FrameWidth - BallDesc.Radius;
				BallVel.Direction.x *= -1;
			}
		}

		for (entity A : Entities)
		{
			for (entity B : Entities)
			{
				if (A == B) continue;
				if (A.HasTag("Ball"))
				{
                    if (B.BelongsToGroup("Brick") || B.HasTag("Player"))
					{
                        if (!B.HasComponent<transform>())
                            continue;
                        if (B.BelongsToGroup("Brick")  && !B.HasComponent<rectangle>())
                            continue;
                        if (B.HasTag("Player") && !B.HasComponent<rectangle>())
                            continue;

                        transform& ATrans = A.GetComponent<transform>();
						circle& BallInfo = A.GetComponent<circle>();
                        vec2 APos = ATrans.Position;
                        float ARadius = BallInfo.Radius;

                        transform& BTrans = B.GetComponent<transform>();
                        rectangle& BInfo = B.GetComponent<rectangle>();
                        vec2 BPos = BTrans.Position;
                        vec2 BDims = BInfo.Dims;

						vec2 Normal;
						float Depth;
                        bool Collision = CircleRectCollision(APos, ARadius, BPos, BDims, Normal, Depth);
                        if (Collision)
                            window::EventsDispatcher.Emit<collision_event>(A, B, Normal, Depth);
                    }
                }
			}
		}
	}
};
