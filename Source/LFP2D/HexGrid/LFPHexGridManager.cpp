// Fill out your copyright notice in the Description page of Project Settings.


#include "LFP2D/HexGrid/LFPHexGridManager.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"

// 六边形方向向量 (平顶六边形布局)
const TArray<FLFPHexCoordinates> ALFPHexGridManager::HexDirections = {
	FLFPHexCoordinates(1, 0),   // 东
	FLFPHexCoordinates(1, -1),  // 东北
	FLFPHexCoordinates(0, -1),  // 西北
	FLFPHexCoordinates(-1, 0),  // 西
	FLFPHexCoordinates(-1, 1),  // 西南
	FLFPHexCoordinates(0, 1)    // 东南
};

// Sets default values
ALFPHexGridManager::ALFPHexGridManager()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	HexSize = 100.0f;
	GridWidth = 10;
	GridHeight = 10;
}

// Called when the game starts or when spawned
void ALFPHexGridManager::BeginPlay()
{
	Super::BeginPlay();

	GenerateGrid(GridWidth, GridHeight);
}

// Called every frame
void ALFPHexGridManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 调试绘制网格
	if (bDrawDebug)
	{
		for (auto& TilePair : GridMap)
		{
			if (TilePair.Value)
			{
				FVector Center = TilePair.Value->GetActorLocation();
				DrawDebugHexagon(Center, FColor::Green);

				// 显示坐标
				FLFPHexCoordinates Coords = TilePair.Value->GetCoordinates();
				FString CoordText = FString::Printf(TEXT("(%d,%d)"), Coords.Q, Coords.R);
				DrawDebugString(GetWorld(), Center + FVector(0, 0, 20), CoordText, nullptr, FColor::White, 0, true);
			}
		}
	}
}

void ALFPHexGridManager::GenerateGrid(int32 Width, int32 Height)
{
	// 清除现有网格
	for (auto& Tile : GridMap)
	{
		if (Tile.Value)
		{
			Tile.Value->Destroy();
		}
	}
	GridMap.Empty();

	// 生成新网格
	for (int32 r = 0; r < Height; r++)
	{
		for (int32 q = 0; q < Width; q++)
		{
			// 创建偏移坐标 - 平顶六边形布局
			int32 offsetQ = q - FMath::FloorToInt(r / 2.0f);

			// 创建六边形坐标
			FLFPHexCoordinates HexCoord(offsetQ, r);

			// 生成世界位置（使用垂直缩放）
			FVector WorldLocation = HexCoord.ToWorldLocation(HexSize, VerticalScale) + GetActorLocation();

			// 生成格子
			FActorSpawnParameters SpawnParams;
			SpawnParams.Owner = this;
			ALFPHexTile* NewTile = GetWorld()->SpawnActor<ALFPHexTile>(
				HexTileClass,
				WorldLocation,
				FRotator::ZeroRotator,
				SpawnParams
			);

			if (NewTile)
			{
				NewTile->SetCoordinates(HexCoord);
				NewTile->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);

				// 添加到网格映射 (使用Q,R作为键)
				FIntPoint Key(HexCoord.Q, HexCoord.R);
				GridMap.Add(Key, NewTile);
			}
		}
	}
}

TArray<ALFPHexTile*> ALFPHexGridManager::GetNeighbors(const FLFPHexCoordinates& Coords) const
{
	TArray<ALFPHexTile*> Neighbors;

	for (const FLFPHexCoordinates& Dir : HexDirections)
	{
		FLFPHexCoordinates NeighborCoords(Coords.Q + Dir.Q, Coords.R + Dir.R);
		FIntPoint Key(NeighborCoords.Q, NeighborCoords.R);

		if (ALFPHexTile* const* FoundTile = GridMap.Find(Key))
		{
			Neighbors.Add(*FoundTile);
		}
	}

	return Neighbors;
}

TArray<ALFPHexTile*> ALFPHexGridManager::GetMovementRange(ALFPHexTile* StartTile, int32 MoveRange)
{
	//// Todo: 用Map优化
	//TPair<ALFPHexTile*, int32> Key(StartTile, MoveRange);
	//if (MoveRangeCache.Contains(Key))
	//{
	//	return MoveRangeCache[Key];
	//}

	TArray<ALFPHexTile*> ReachableTiles;
	if (!StartTile || MoveRange <= 0) return ReachableTiles;

	// BFS数据结构
	TMap<ALFPHexTile*, int32> MoveCosts;
	TQueue<ALFPHexTile*> TileQueue;

	TileQueue.Enqueue(StartTile);
	MoveCosts.Add(StartTile, 0);
	ReachableTiles.Add(StartTile);

	while (!TileQueue.IsEmpty())
	{
		ALFPHexTile* CurrentTile;
		TileQueue.Dequeue(CurrentTile);
		int32 CurrentCost = MoveCosts[CurrentTile];

		// 获取所有邻居
		for (ALFPHexTile* Neighbor : GetNeighbors(CurrentTile->GetCoordinates()))
		{
			if (!Neighbor || !Neighbor->IsWalkable() || Neighbor->IsOccupied()) continue;

			// 计算新移动代价
			const int32 NewCost = CurrentCost + 1;

			// 如果尚未访问或找到更短路径
			if (!MoveCosts.Contains(Neighbor) || NewCost < MoveCosts[Neighbor])
			{
				MoveCosts.Add(Neighbor, NewCost);

				// 如果在移动范围内
				if (NewCost <= MoveRange)
				{
					ReachableTiles.Add(Neighbor);
					TileQueue.Enqueue(Neighbor);
				}
			}
		}
	}
	ReachableTiles.Remove(StartTile);
	return ReachableTiles;
}

ALFPHexTile* ALFPHexGridManager::GetTileAtCoordinates(const FLFPHexCoordinates& Coords) const
{
	FIntPoint Key(Coords.Q, Coords.R);
	if (ALFPHexTile* const* FoundTile = GridMap.Find(Key))
	{
		return *FoundTile;
	}
	return nullptr;
}

TArray<ALFPHexTile*> ALFPHexGridManager::FindPath(ALFPHexTile* Start, ALFPHexTile* End)
{
	TArray<ALFPHexTile*> Path;

	// 验证输入有效性
	if (!Start || !End || !End->IsWalkable())
	{
		UE_LOG(LogTemp, Warning, TEXT("FindPath: Invalid start or end tile"));
		return Path;
	}

	// 起点终点相同的情况
	if (Start == End) {
		Path.Add(Start);
		UE_LOG(LogTemp, Log, TEXT("FindPath: Start and end are the same tile"));
		return Path;
	}

	// 寻路数据结构
	TMap<ALFPHexTile*, ALFPHexTile*> ParentMap;
	TMap<ALFPHexTile*, float> GScoreMap;  // 实际移动代价
	TSet<ALFPHexTile*> OpenSet;
	TSet<ALFPHexTile*> ClosedSet;

	// 初始化起点
	OpenSet.Add(Start);
	GScoreMap.Add(Start, 0.0f);
	ParentMap.Add(Start, nullptr);

	bool bPathFound = false;

	while (!OpenSet.IsEmpty())
	{
		// 获取开放集中F值最小的节点
		ALFPHexTile* Current = nullptr;
		float LowestF = TNumericLimits<float>::Max();

		for (ALFPHexTile* Tile : OpenSet)
		{
			const float G = GScoreMap[Tile];
			const float H = FLFPHexCoordinates::Distance(Tile->GetCoordinates(), End->GetCoordinates());
			const float F = G + H;

			if (F < LowestF)
			{
				LowestF = F;
				Current = Tile;
			}
		}

		// 到达终点
		if (Current == End)
		{
			bPathFound = true;
			ALFPHexTile* PathNode = End;
			while (PathNode)
			{
				Path.Add(PathNode);
				PathNode = ParentMap[PathNode];
			}
			Algo::Reverse(Path);
			break;
		}

		// 移动节点到关闭集
		OpenSet.Remove(Current);
		ClosedSet.Add(Current);

		// 检查所有邻居
		for (const FLFPHexCoordinates& Dir : HexDirections)
		{
			FLFPHexCoordinates NeighborCoords = Current->GetCoordinates();
			NeighborCoords.Q += Dir.Q;
			NeighborCoords.R += Dir.R;
			NeighborCoords.S = -NeighborCoords.Q - NeighborCoords.R;  // 更新立方坐标

			ALFPHexTile* Neighbor = GetTileAtCoordinates(NeighborCoords);

			// 跳过无效邻居
			if (!Neighbor ||
				!Neighbor->IsWalkable() ||
				Neighbor->IsOccupied() ||
				ClosedSet.Contains(Neighbor))
			{
				continue;
			}

			// 计算新G值（假设所有格子代价相同）
			const float TentativeG = GScoreMap[Current] + 1.0f;

			// 发现新节点或找到更优路径
			if (!OpenSet.Contains(Neighbor) || TentativeG < GScoreMap[Neighbor])
			{
				ParentMap.Add(Neighbor, Current);
				GScoreMap.Add(Neighbor, TentativeG);

				if (!OpenSet.Contains(Neighbor)) {
					OpenSet.Add(Neighbor);
				}
			}
		}
	}

	Path.Remove(Start);
	// 调试绘制路径
	//DrawDebugPath(Path, bPathFound);

	if (bPathFound) {
		UE_LOG(LogTemp, Log, TEXT("FindPath: Found path with %d tiles"), Path.Num());
	}
	else {
		UE_LOG(LogTemp, Warning, TEXT("FindPath: No valid path found"));
	}

	return Path;
}

void ALFPHexGridManager::DrawDebugHexagon(const FVector& Center, FColor Color) const
{
	const float AngleStep = 60.0f;
	const float HorizontalRadius = HexSize; // 水平方向半径不变
	const float VerticalRadius = HexSize * VerticalScale; // 垂直方向半径缩放

	// 添加 30° 偏移，使六边形正确对齐
	const float AngleOffset = 30.0f;

	TArray<FVector> Points;
	for (int32 i = 0; i < 6; i++)
	{
		// 添加 30° 偏移
		float Angle = FMath::DegreesToRadians(AngleStep * i + AngleOffset);
		FVector Point(
			Center.X + HorizontalRadius * FMath::Cos(Angle),
			Center.Y + VerticalRadius * FMath::Sin(Angle), // 应用垂直缩放
			Center.Z
		);
		Points.Add(Point);
	}

	// 连接点形成六边形
	for (int32 i = 0; i < 6; i++)
	{
		int32 NextIndex = (i + 1) % 6;
		DrawDebugLine(GetWorld(), Points[i], Points[NextIndex], Color, false, -1, 0, 2);
	}
}

void ALFPHexGridManager::DrawDebugPath(const TArray<ALFPHexTile*>& Path, bool bPathFound)
{
	if (!GetWorld()) return;

	const float DebugDuration = 5.0f; // 显示5秒

	// 绘制起点和终点
	if (Path.Num() > 0) {
		// 起点：绿色球体
		DrawDebugSphere(
			GetWorld(),
			Path[0]->GetActorLocation() + FVector(0, 0, 20),
			15.0f, 12, FColor::Green, false, DebugDuration
		);
	}

	if (bPathFound && Path.Num() > 1) {
		// 终点：蓝色球体
		DrawDebugSphere(
			GetWorld(),
			Path.Last()->GetActorLocation() + FVector(0, 0, 20),
			15.0f, 12, FColor::Blue, false, DebugDuration
		);
	}

	// 绘制路径连线
	for (int32 i = 0; i < Path.Num() - 1; i++)
	{
		FVector StartLoc = Path[i]->GetActorLocation() + FVector(0, 0, 15);
		FVector EndLoc = Path[i + 1]->GetActorLocation() + FVector(0, 0, 15);

		// 路径线：黄色
		DrawDebugLine(
			GetWorld(),
			StartLoc,
			EndLoc,
			FColor::Yellow,
			false,
			DebugDuration,
			0,
			3.0f // 线宽
		);

		// 路径点：黄色球体
		DrawDebugSphere(
			GetWorld(),
			StartLoc,
			10.0f, 8, FColor::Yellow, false, DebugDuration
		);
	}

	// 如果找到路径但在绘制前就被清空了（比如长度为1）
	if (bPathFound && Path.Num() == 1) {
		DrawDebugSphere(
			GetWorld(),
			Path[0]->GetActorLocation() + FVector(0, 0, 20),
			15.0f, 12, FColor::Blue, false, DebugDuration
		);
	}

	// 绘制失败提示
	if (!bPathFound) {
		// 在起点位置绘制红色X
		FVector StartLoc = Path.Num() > 0 ?
			Path[0]->GetActorLocation() :
			(GetActorLocation() + FVector(0, 0, 100));

		DrawDebugLine(
			GetWorld(),
			StartLoc + FVector(-30, -30, 50),
			StartLoc + FVector(30, 30, 50),
			FColor::Red, false, DebugDuration, 0, 5.0f
		);
		DrawDebugLine(
			GetWorld(),
			StartLoc + FVector(-30, 30, 50),
			StartLoc + FVector(30, -30, 50),
			FColor::Red, false, DebugDuration, 0, 5.0f
		);
	}
}

// 主入口函数：通过屏幕位置获取六边形Tile
ALFPHexTile* ALFPHexGridManager::GetHexTileUnderCursor(const FVector2D& ScreenPosition, APlayerController* PlayerController)
{
	FHitResult Hit;
	bool bHitSuccessful = false;
	bHitSuccessful = PlayerController->GetHitResultUnderCursor(ECollisionChannel::ECC_Visibility, true, Hit);
	if (!bHitSuccessful)
	{
		return nullptr;
	}

	// 转换为局部坐标（相对于网格管理器
	FVector2D LocalLocation;
	LocalLocation.X = Hit.Location.X - GetActorLocation().X;
	LocalLocation.Y = Hit.Location.Y - GetActorLocation().Y;

	// 计算六边形坐标
	FLFPHexCoordinates HexCoords = FLFPHexCoordinates::FromWorldLocation(
		LocalLocation,
		HexSize,
		VerticalScale
	);

	// 在GridMap中查找
	FIntPoint Key(HexCoords.Q, HexCoords.R);
	if (GridMap.Contains(Key))
		return GridMap[Key];

	return nullptr;
}

// 世界坐标转换为六边形网格坐标
FVector2D ALFPHexGridManager::WorldToHexGridPosition(const FVector& WorldPosition, float InHexSize, float InVerticalScale)
{
	// 尖顶六边形布局参数
	const float HorizontalSpacing = InHexSize * 1.5f * InVerticalScale; // 水平间距 (应用垂直缩放)
	const float VerticalSpacing = InHexSize * FMath::Sqrt(3.0f); // 垂直间距

	// 网格原点位置
	FVector GridOrigin = GetActorLocation();

	// 计算相对于网格原点的位置
	float RelX = WorldPosition.X - GridOrigin.X;
	float RelY = WorldPosition.Y - GridOrigin.Y;

	// 转换为六边形网格坐标（无量纲）
	return FVector2D(
		RelX / HorizontalSpacing,
		RelY / VerticalSpacing
	);
}

// 像素位置转换为六边形坐标
FLFPHexCoordinates ALFPHexGridManager::PixelToHexCoordinates(const FVector2D& PixelPosition, float InHexSize)
{
    // 提取坐标值
    float x = PixelPosition.X;
    float y = PixelPosition.Y;
    
    // 转换为立方体坐标 - 尖顶布局公式
    float q = (2.0f / 3.0f) * x;
    float r = (-1.0f / 3.0f) * x + (FMath::Sqrt(3.0f) / 3.0f) * y;
    
    // 使用优化的六边形坐标四舍五入
    return RoundToHex(q, r);
}

// 检查点是否在六边形内
bool ALFPHexGridManager::IsPointInHexagon(const FVector2D& Point, const FVector2D& Center, float InHexSize)
{
	// 将点转换为相对于六边形中心的位置
	FVector2D relPoint = Point - Center;

	// 尖顶六边形的顶点位置（从0度开始）
	static const TArray<FVector2D> vertices = [] {
		TArray<FVector2D> result;
		for (int i = 0; i < 6; i++) {
			float angle_deg = 60 * i; // 尖顶六边形从0度开始
			float angle_rad = FMath::DegreesToRadians(angle_deg);
			result.Add(FVector2D(FMath::Cos(angle_rad), FMath::Sin(angle_rad)));
		}
		return result;
		}();

	// 检查点是否在六边形内（使用射线法）
	int crossings = 0;
	for (int i = 0; i < 6; i++) {
		int j = (i + 1) % 6;
		FVector2D v1 = vertices[i] * InHexSize;
		FVector2D v2 = vertices[j] * InHexSize;

		// 检查边是否跨越点的y坐标
		if ((v1.Y > relPoint.Y) != (v2.Y > relPoint.Y)) {
			// 计算交点x坐标
			float intersectX = (v2.X - v1.X) * (relPoint.Y - v1.Y) / (v2.Y - v1.Y) + v1.X;

			// 如果交点在点的右侧
			if (relPoint.X < intersectX) {
				crossings++;
			}
		}
	}

	// 奇数次跨越表示点在内部
	return (crossings % 2 == 1);
}

// 优化的六边形坐标四舍五入
FLFPHexCoordinates ALFPHexGridManager::RoundToHex(float q, float r)
{
	// 转换为立方体坐标
	float cube_x = q;
	float cube_z = r;
	float cube_y = -cube_x - cube_z;

	// 四舍五入到最近的整数
	int32 rx = FMath::RoundToInt(cube_x);
	int32 ry = FMath::RoundToInt(cube_y);
	int32 rz = FMath::RoundToInt(cube_z);

	// 检查四舍五入误差
	float x_diff = FMath::Abs(rx - cube_x);
	float y_diff = FMath::Abs(ry - cube_y);
	float z_diff = FMath::Abs(rz - cube_z);

	// 调整最大误差的坐标
	if (x_diff > y_diff && x_diff > z_diff) {
		rx = -ry - rz;
	}
	else if (y_diff > z_diff) {
		ry = -rx - rz;
	}
	else {
		rz = -rx - ry;
	}

	return FLFPHexCoordinates(rx, rz);
}