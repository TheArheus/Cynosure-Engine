#include "path_finding_visualizer_game_module.h"

#include "core/asset_manager/asset_store.cpp"
#include "core/entity_component_system/entity_systems.cpp"

static constexpr float CELL_SIZE    = 10.0f;
static constexpr float CELL_SPACING = 2.0f;

void path_finding_visualizer::
ModuleStart()
{
	Grid.resize(GridHeight, std::vector<cell_type>(GridWidth, cell_type::empty));
	CameFrom.resize(GridHeight, std::vector<std::pair<int,int>>(GridWidth, { -1, -1 }));
	CostG.resize(GridHeight, std::vector<int>(GridWidth, INT_MAX));

	VertexBuffer = Gfx.GpuMemoryHeap->CreateBuffer("Vertex Buffer", sizeof(prim_vert), 128, RF_StorageBuffer);
	IndexBuffer  = Gfx.GpuMemoryHeap->CreateBuffer("Index Buffer" , sizeof(u32)      , 128, RF_IndexBuffer);

	EventsDispatcher.Subscribe(this, &path_finding_visualizer::OnButtonHold);
}

void path_finding_visualizer::
ModuleUpdate()
{
	RenderUI();

    if(IsSearching)
    {
        bool IsDone = false;
        switch(CurrentAlgorithm)
        {
            case pathfinding_algorithm::bfs:   IsDone = BfsStep();   break;
            case pathfinding_algorithm::dfs:   IsDone = DfsStep();   break;
            case pathfinding_algorithm::astar: IsDone = AStarStep(); break;
            default: break;
        }
        if(IsDone)
        {
			int CurR = TargetPos.y;
			int CurC = TargetPos.x;
			while (!(CurR == (int)StartPos.y && CurC == (int)StartPos.x))
			{
				if (Grid[CurR][CurC] != cell_type::start &&
					Grid[CurR][CurC] != cell_type::target)
				{
					Grid[CurR][CurC] = cell_type::path;
				}
				auto Next = CameFrom[CurR][CurC];
				CurR = Next.first;
				CurC = Next.second;
			}
            IsSearching = false;
			StartUsed = false;
        }
    }

    float TotalGridWidth  = GridWidth  * (CELL_SIZE + CELL_SPACING);
    float TotalGridHeight = GridHeight * (CELL_SIZE + CELL_SPACING);

    float LeftMargin   = 50.0f;
    float RightMargin  = 50.0f;
    float BottomMargin = 18.0f;

    float AvailableWidth = Window.Width - (LeftMargin + RightMargin);

    OffsetX = LeftMargin + (AvailableWidth - TotalGridWidth) * 0.5f;
    if (OffsetX < LeftMargin) OffsetX = LeftMargin;

    //OffsetY = BottomMargin;
    OffsetY = (Window.Height - TotalGridHeight) * 0.5f;

    for (u32 Row = 0; Row < GridHeight; Row++)
    {
        for (u32 Col = 0; Col < GridWidth; Col++)
        {
            float CellX = OffsetX + Col * (CELL_SIZE + CELL_SPACING);
            float CellY = OffsetY + Row * (CELL_SIZE + CELL_SPACING);
            // float CellY = OffsetY + (GridHeight - 1 - Row) * (CELL_SIZE + CELL_SPACING);

            vec3 Color(1.0f);
			switch (Grid[Row][Col])
			{
				case cell_type::wall:    Color = vec3(1.0f, 0.0f, 0.0f); break;
				case cell_type::start:   Color = vec3(0.0f, 1.0f, 0.0f); break;
				case cell_type::target:  Color = vec3(0.0f, 0.0f, 1.0f); break;
				case cell_type::visited: Color = vec3(0.7f, 0.7f, 0.7f); break;
				case cell_type::path:    Color = vec3(1.0f, 1.0f, 0.0f); break;
				default:                 Color = vec3(1.0f);             break;
			}

            PushRectangle(Vertices, Indices, vec2(CellX, CellY) + 0.5f * vec2(CELL_SIZE), vec2(CELL_SIZE), Color);
        }
    }

	{
		Gfx.GpuMemoryHeap->UpdateBuffer(VertexBuffer, Vertices.data(), sizeof(prim_vert), Vertices.size());
		Gfx.GpuMemoryHeap->UpdateBuffer(IndexBuffer , Indices.data() , sizeof(u32), Indices.size());

		primitive_2d::raster_parameters RasterParameters = {};
		RasterParameters.IndexBuffer = IndexBuffer;
		RasterParameters.ColorTarget = Gfx.ColorTarget[Gfx.BackBufferIndex];

		primitive_2d::parameters Parameters = {};
		Parameters.Vertices = VertexBuffer;
		Parameters.Texture  = Gfx.ColorTarget[Gfx.BackBufferIndex]; // NOTE: this is temporary. I need to implement so that if I don't bind a texture there would be a null texture

		Gfx.AddRasterPass<primitive_2d>("Primitive Rendering", Parameters, RasterParameters, [IndexCount = Indices.size(), FramebufferDims = vec2(Window.Width, Window.Height)](command_list* Cmd)
		{
			Cmd->SetConstant((void*)FramebufferDims.E, sizeof(vec2));
			Cmd->DrawIndexed(0, IndexCount, 0, 0, 1);
		});
	}

	Vertices.clear();
	Indices.clear();
}

void path_finding_visualizer::
ResetPathfindingStates()
{
    for (u32 Row = 0; Row < GridHeight; Row++)
    {
        for (u32 Col = 0; Col < GridWidth; Col++)
        {
			if(Grid[Row][Col] == cell_type::visited || Grid[Row][Col] == cell_type::path) Grid[Row][Col] = cell_type::empty;
        }
    }
}

std::vector<std::pair<int, int>> path_finding_visualizer::
NeighborsOf(int Row, int Col)
{
    std::vector<std::pair<int, int>> Neighbors;

    static const int RowDirs[4] = { +1, -1,  0,  0 };
    static const int ColDirs[4] = {  0,  0, +1, -1 };

    for(int i = 0; i < 4; i++)
    {
        int NewRow = Row + RowDirs[i];
        int NewCol = Col + ColDirs[i];

        if(NewRow >= 0 && NewRow < (int)GridHeight &&
           NewCol >= 0 && NewCol < (int)GridWidth)
        {
            Neighbors.push_back({NewRow, NewCol});
        }
    }

    return Neighbors;
}

bool path_finding_visualizer::
BfsStep()
{
	if (!StartUsed)
	{
		Frontier.push(std::pair<int, int>(StartPos.y, StartPos.x));
		StartUsed = true;
	}

    if (Frontier.empty())
    {
        return true; 
    }

	auto Top = Frontier.front();
	Frontier.pop();
	for (auto Neighbor : NeighborsOf(Top.first, Top.second))
	{
		if (Grid[Neighbor.first][Neighbor.second] == cell_type::target)
		{
			CameFrom[Neighbor.first][Neighbor.second] = {Top.first, Top.second};
			return true;
		}
		if (Grid[Neighbor.first][Neighbor.second] == cell_type::empty)
		{
			CameFrom[Neighbor.first][Neighbor.second] = {Top.first, Top.second};
			Grid[Neighbor.first][Neighbor.second] = cell_type::visited;
			Frontier.push(Neighbor);
		}
	}

	return false;
}

bool path_finding_visualizer::
DfsStep()
{
	if (!StartUsed)
	{
		Stack.push(std::pair<int, int>(StartPos.y, StartPos.x));
		StartUsed = true;
	}

    if (Stack.empty())
    {
        return true; 
    }

	auto Top = Stack.top();
	Stack.pop();
	for (auto Neighbor : NeighborsOf(Top.first, Top.second))
	{
		if (Grid[Neighbor.first][Neighbor.second] == cell_type::target)
		{
			CameFrom[Neighbor.first][Neighbor.second] = {Top.first, Top.second};
			return true;
		}
		if (Grid[Neighbor.first][Neighbor.second] == cell_type::empty)
		{
			CameFrom[Neighbor.first][Neighbor.second] = {Top.first, Top.second};
			Grid[Neighbor.first][Neighbor.second] = cell_type::visited;
			Stack.push(Neighbor);
		}
	}

	return false;
}

bool path_finding_visualizer::
AStarStep()
{
	if (!StartUsed)
	{
        astar_node StartNode;
        StartNode.Row = (int)StartPos.y;
        StartNode.Col = (int)StartPos.x;
        StartNode.CostG = 0;
		int CostH = ComputeHeuristic(CurrentHeuristic, StartNode.Row, StartNode.Col, (int)TargetPos.y, (int)TargetPos.x);
        StartNode.CostF = StartNode.CostG + CostH;

        OpenSet.push(StartNode);

        CostG[StartNode.Row][StartNode.Col] = 0;
		OpenSet.push(StartNode);

		StartUsed = true;
	}

    if (OpenSet.empty())
    {
        return true;
    }

    astar_node Top = OpenSet.top();
    OpenSet.pop();

    if (Grid[Top.Row][Top.Col] == cell_type::target)
    {
        return true;
    }

    if (Grid[Top.Row][Top.Col] == cell_type::empty)
    {
        Grid[Top.Row][Top.Col] = cell_type::visited;
    }

    for (auto Neighbor : NeighborsOf(Top.Row, Top.Col))
    {
        if (Grid[Neighbor.first][Neighbor.second] == cell_type::wall) continue;

        int NewCostG = Top.CostG + 1;

        if (NewCostG < CostG[Neighbor.first][Neighbor.second])
        {
            CostG[Neighbor.first][Neighbor.second] = NewCostG;
            CameFrom[Neighbor.first][Neighbor.second] = { Top.Row, Top.Col };

            int CostH = ComputeHeuristic(CurrentHeuristic, Neighbor.first, Neighbor.second, (int)TargetPos.y, (int)TargetPos.x);
            int CostF = NewCostG + CostH;

            if (Grid[Neighbor.first][Neighbor.second] == cell_type::target)
            {
                return true;
            }

            if (Grid[Neighbor.first][Neighbor.second] == cell_type::empty)
            {
                Grid[Neighbor.first][Neighbor.second] = cell_type::visited;
            }

            astar_node NeighborNode;
            NeighborNode.Row = Neighbor.first;
            NeighborNode.Col = Neighbor.second;
            NeighborNode.CostG = NewCostG;
            NeighborNode.CostF = CostF;
            OpenSet.push(NeighborNode);
        }
    }

	return false;
}

void path_finding_visualizer::
RenderUI()
{
    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(Window.Width, Window.Height), ImGuiCond_Always);
    
    ImGuiWindowFlags WindowFlags  = ImGuiWindowFlags_NoTitleBar |
                                    ImGuiWindowFlags_NoResize |
                                    ImGuiWindowFlags_NoMove |
                                    ImGuiWindowFlags_NoScrollbar |
                                    ImGuiWindowFlags_NoScrollWithMouse |
                                    ImGuiWindowFlags_NoCollapse |
                                    ImGuiWindowFlags_NoBackground;

    ImGui::Begin("Path Finding Controls", nullptr, WindowFlags);

    if(ImGui::RadioButton("Wall", EditMode == edit_mode::wall))
        EditMode = edit_mode::wall;
    if(ImGui::RadioButton("Start", EditMode == edit_mode::start))
        EditMode = edit_mode::start;
    if(ImGui::RadioButton("Target", EditMode == edit_mode::target))
        EditMode = edit_mode::target;
    if(ImGui::RadioButton("Eraser", EditMode == edit_mode::eraser))
        EditMode = edit_mode::eraser;

    if(ImGui::Button("Run BFS"))
    {
        CurrentAlgorithm = pathfinding_algorithm::bfs;
        ResetPathfindingStates();

        IsSearching = true;

		while(!Frontier.empty()) Frontier.pop();

		CameFrom.clear();
		CameFrom.resize(GridHeight, std::vector<std::pair<int,int>>(GridWidth, { -1, -1 }));
    }
    if(ImGui::Button("Run DFS"))
    {
        CurrentAlgorithm = pathfinding_algorithm::dfs;
        ResetPathfindingStates();

        IsSearching = true;

		while(!Stack.empty()) Stack.pop();

		CameFrom.clear();
		CameFrom.resize(GridHeight, std::vector<std::pair<int,int>>(GridWidth, { -1, -1 }));
    }
    if(ImGui::Button("Run A*"))
    {
        CurrentAlgorithm = pathfinding_algorithm::astar;
        ResetPathfindingStates();

        IsSearching = true;

		while(!OpenSet.empty()) OpenSet.pop();

		CameFrom.clear();
		CameFrom.resize(GridHeight, std::vector<std::pair<int,int>>(GridWidth, { -1, -1 }));
		CostG.clear();
		CostG.resize(GridHeight, std::vector<int>(GridWidth, INT_MAX));
    }
	ImGui::Text("Heuristic:");

	if (ImGui::RadioButton("Manhattan", CurrentHeuristic == heuristic_type::manhattan))
		CurrentHeuristic = heuristic_type::manhattan;
	if (ImGui::RadioButton("Euclidean", CurrentHeuristic == heuristic_type::euclidean))
		CurrentHeuristic = heuristic_type::euclidean;
	if (ImGui::RadioButton("Diagonal (Chebyshev)", CurrentHeuristic == heuristic_type::diagonal))
		CurrentHeuristic = heuristic_type::diagonal;

    ImGui::End();
}

bool path_finding_visualizer::
GetMouseCell(float MouseX, float MouseY, int& OutRow, int& OutCol)
{
    float RelX = MouseX - OffsetX;
    float RelY = MouseY - OffsetY;
    if (RelX < 0.0f || RelY < 0.0f)
        return false;

    OutCol = static_cast<int>(std::floor(RelX / (CELL_SIZE + CELL_SPACING)));
    OutRow = static_cast<int>(std::floor(RelY / (CELL_SIZE + CELL_SPACING)));

    if (OutCol < 0 || OutCol >= static_cast<int>(GridWidth) ||
        OutRow < 0 || OutRow >= static_cast<int>(GridHeight))
    {
        return false;
    }

    float InCellX = fmod(RelX, (CELL_SIZE + CELL_SPACING));
    float InCellY = fmod(RelY, (CELL_SIZE + CELL_SPACING));

    if (InCellX > CELL_SIZE || InCellY > CELL_SIZE)
    {
        return false;
    }

    return true;
}

void path_finding_visualizer::
OnButtonHold(key_hold_event& Event)
{
	float MouseX = Window.MouseX;
	float MouseY = Window.MouseY;
	if(Event.Code == EC_LBUTTON)
	{
		int Col = 0;
        int Row = 0;
        if(GetMouseCell(MouseX, MouseY, Row, Col))
        {
            switch(EditMode)
            {
                case edit_mode::wall:
                    Grid[Row][Col] = cell_type::wall;
                    break;
                case edit_mode::start:
                    StartPos = vec2(Col, Row);
                    Grid[Row][Col] = cell_type::start;
                    break;
                case edit_mode::target:
                    TargetPos = vec2(Col, Row);
                    Grid[Row][Col] = cell_type::target;
                    break;
                case edit_mode::eraser:
                    Grid[Row][Col] = cell_type::empty;
                    break;
            }
        }
	}
}

extern "C" GameModuleCreateFunc(GameModuleCreate)
{
	ImGui::SetCurrentContext(NewWindow.imguiContext.get());
	GlobalGuiContext = NewContext;
	game_module* Ptr = new path_finding_visualizer(NewWindow, NewEventDispatcher, NewRegistry, NewGfx);
	return Ptr;
}
