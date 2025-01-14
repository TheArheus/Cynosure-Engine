#pragma once

#define USE_CE_GUI 0

// TODO: Make this modules as independent as possible
//       so that there would be only game dependent sources

#include "intrinsics.h"
#include "core/entity_component_system/entity_systems.h"
#include "core/events/events.hpp"
#include "core/asset_manager/asset_store.h"
#include "core/gfx/renderer.h"
#include "core/platform/window.hpp"
#include "core/ce_gui.h"
#include "game_module.hpp"

#include <stack>
#include <queue>


enum class edit_mode
{
    wall,
    start,
    target,
    eraser
};

enum class pathfinding_algorithm
{
    none,
    bfs,
    dfs,
    astar
};

enum class cell_type
{
    empty,
    wall,
    start,
    target,
    visited,
    path
};

enum class heuristic_type
{
    manhattan,
    euclidean,
    diagonal
};

struct astar_node
{
	int Row;
	int Col;
	int CostG; // NOTE: Distance from start
	int CostF; // NOTE: CostG + CostH

	bool operator>(const astar_node& oth) const
	{
		return CostF > oth.CostF;
	}
};

inline int ComputeHeuristic(heuristic_type Type, int Row, int Col, int TargetRow, int TargetCol)
{
    switch (Type)
    {
        case heuristic_type::manhattan:
        {
            // |x1 - x2| + |y1 - y2|
            return std::abs(Row - TargetRow) + std::abs(Col - TargetCol);
        }
        case heuristic_type::euclidean:
        {
            // sqrt((x1 - x2)^2 + (y1 - y2)^2)
            // But we can return an integer => either cast or keep float
            // For consistent integer usage, some people skip the sqrt.
            // However, if you skip sqrt, ensure it doesn't overestimate.
            // Actually sqrt won't overestimate, so let's do it properly as float
            float DiffR = (float)(Row - TargetRow);
            float DiffC = (float)(Col - TargetCol);
            float Dist = std::sqrt(DiffR*DiffR + DiffC*DiffC);
            // Typically you might keep a float, but let's do int rounding:
            return (int)(Dist + 0.5f);
        }
        case heuristic_type::diagonal:
        {
            // Chebyshev distance: max(|dx|, |dy|)
            int dx = std::abs(Row - TargetRow);
            int dy = std::abs(Col - TargetCol);
            return std::max(dx, dy);
        }
    }

    return 0;
}

class path_finding_visualizer : public game_module
{
	edit_mode EditMode;
	pathfinding_algorithm CurrentAlgorithm;
	heuristic_type CurrentHeuristic;

	resource_descriptor VertexBuffer;
	resource_descriptor IndexBuffer;

	std::vector<prim_vert> Vertices;
	std::vector<u32> Indices;

	bool IsSearching = false;
	bool StartUsed   = false;

	std::vector<std::vector<cell_type>> Grid;

	u32 GridWidth  = 65;
	u32 GridHeight = 65;

	float OffsetX;
	float OffsetY;

	vec2 StartPos;
	vec2 TargetPos;

    std::queue<std::pair<int,int>> Frontier;
    std::stack<std::pair<int,int>> Stack;
	std::priority_queue<astar_node, std::vector<astar_node>, std::greater<astar_node>> OpenSet;

	std::vector<std::vector<std::pair<int,int>>> CameFrom;
	std::vector<std::vector<int>> CostG;

	void RenderUI();
	void ResetPathfindingStates();
	std::vector<std::pair<int, int>> NeighborsOf(int Row, int Col);

	bool GetMouseCell(float MouseX, float MouseY, int& OutRow, int& OutCol);
	void OnButtonHold(key_hold_event& Event);

	bool BfsStep();
	bool DfsStep();
	bool AStarStep();

public:
	Construct(path_finding_visualizer);

    void ModuleStart() override;
    void ModuleUpdate() override;
};
