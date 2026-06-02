#include "LFP2D/UI/Fighting/LFPCardDragLineWidget.h"

#include "PaperSprite.h"
#include "Rendering/DrawElements.h"
#include "Styling/WidgetStyle.h"

ULFPCardDragLineWidget::ULFPCardDragLineWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	LineBrush.DrawAs = ESlateBrushDrawType::Image;
	LineBrush.Tiling = ESlateBrushTileType::Horizontal;
	LineBrush.ImageType = ESlateBrushImageType::FullColor;
	SetVisibility(ESlateVisibility::HitTestInvisible);
	ForceVolatile(true);
}

void ULFPCardDragLineWidget::SetLineSprite(UPaperSprite* InSprite)
{
	LineSprite = InSprite;
	LineBrush.SetResourceObject(InSprite);
	LineBrush.DrawAs = InSprite ? ESlateBrushDrawType::Image : ESlateBrushDrawType::NoDrawType;
	LineBrush.Tiling = bTileSpriteAlongLine ? ESlateBrushTileType::Horizontal : ESlateBrushTileType::NoTile;

	if (InSprite)
	{
		const FSlateAtlasData AtlasData = InSprite->GetSlateAtlasData();
		LineBrush.ImageSize = AtlasData.GetSourceDimensions();
	}
	else
	{
		LineBrush.ImageSize = FVector2D::ZeroVector;
	}
}

void ULFPCardDragLineWidget::ConfigureLine(float InLineThickness, FLinearColor InLineTint,
	bool bInTileSpriteAlongLine)
{
	LineThickness = FMath::Max(1.0f, InLineThickness);
	LineTint = InLineTint;
	bTileSpriteAlongLine = bInTileSpriteAlongLine;
	LineBrush.Tiling = bTileSpriteAlongLine ? ESlateBrushTileType::Horizontal : ESlateBrushTileType::NoTile;
}

void ULFPCardDragLineWidget::SetLine(FVector2D InStart, FVector2D InEnd)
{
	Start = InStart;
	End = InEnd;
}

int32 ULFPCardDragLineWidget::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId,
	const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	const int32 MaxLayer = Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId,
		InWidgetStyle, bParentEnabled);

	if (LineBrush.DrawAs == ESlateBrushDrawType::NoDrawType || !LineBrush.GetResourceObject())
	{
		return MaxLayer;
	}

	const FVector2D Delta = End - Start;
	const float Length = Delta.Size();
	if (Length <= KINDA_SMALL_NUMBER)
	{
		return MaxLayer;
	}

	const float Thickness = FMath::Max(1.0f, LineThickness);
	const FVector2D DrawSize(Length, Thickness);
	const FVector2D DrawPosition = Start - FVector2D(0.0f, Thickness * 0.5f);
	const float Angle = FMath::Atan2(Delta.Y, Delta.X);

	FSlateDrawElement::MakeRotatedBox(
		OutDrawElements,
		MaxLayer + 1,
		AllottedGeometry.ToPaintGeometry(DrawSize, FSlateLayoutTransform(DrawPosition)),
		&LineBrush,
		ESlateDrawEffect::None,
		Angle,
		FVector2D(0.0f, 0.5f),
		FSlateDrawElement::RelativeToElement,
		InWidgetStyle.GetColorAndOpacityTint() * LineTint);

	return MaxLayer + 1;
}
