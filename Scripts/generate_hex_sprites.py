"""
生成六边形高亮系统所需的两张贴图：
1. T_HexEdge.png — 边线精灵（白色水平线段，带柔和边缘）
2. T_HexOverlay.png — 六边形覆盖层（白色六边形，带半透明填充）

生成后导入 UE，创建 PaperSprite 资产，配置到 GridManager 蓝图上。
"""

from PIL import Image, ImageDraw
import math
import os
import random

OUTPUT_DIR = r"D:\UE_Projects\LFP2D\Content\Art\HexGrid"

# ===== 参数 =====
# HexSize = 100 (UE 单位)，PixelsPerUnit 默认 = 1
# 平顶六边形边长 = HexSize = 100px
HEX_SIZE = 100
VERTICAL_SCALE = 1.0

# 边长计算：两个相邻顶点间距
# 顶点角度 = 60*i + 30, i=0..5
# V0 = (100*cos30, 100*sin30) = (86.6, 50)
# V1 = (100*cos90, 100*sin90) = (0, 100)
# 边长 = distance(V0, V1) = sqrt(86.6^2 + 50^2) = 100 (等边六边形时)
EDGE_LENGTH = HEX_SIZE  # 等边六边形时边长 = HexSize


def generate_edge_sprite():
    """
    生成边线精灵：一条白色水平线段
    宽度 = 边长(100px)，高度 = 4px，带1px抗锯齿边缘
    """
    width = EDGE_LENGTH
    height = 6  # 4px 实线 + 1px 上下柔和边缘

    img = Image.new('RGBA', (width, height), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)

    # 中间4行为实线（白色），上下各1行为半透明柔和边缘
    for x in range(width):
        # 上边缘柔和
        img.putpixel((x, 0), (255, 255, 255, 80))
        # 实线
        for y in range(1, height - 1):
            img.putpixel((x, y), (255, 255, 255, 255))
        # 下边缘柔和
        img.putpixel((x, height - 1), (255, 255, 255, 80))

    path = os.path.join(OUTPUT_DIR, "T_HexEdge.png")
    img.save(path)
    print(f"生成边线贴图: {path} ({width}x{height})")


def generate_hex_overlay():
    """
    生成六边形覆盖层：白色填充的六边形，匹配平顶布局
    尺寸需要覆盖整个六边形范围
    """
    # 六边形边界框
    # 平顶六边形：宽 = 2*HexSize*cos(30°) ≈ 173, 高 = 2*HexSize
    hex_width = int(2 * HEX_SIZE * math.cos(math.radians(30))) + 4  # +4 余量
    hex_height = int(2 * HEX_SIZE * VERTICAL_SCALE) + 4

    img = Image.new('RGBA', (hex_width, hex_height), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)

    cx = hex_width / 2
    cy = hex_height / 2

    # 计算六边形顶点（平顶，30°偏移）
    points = []
    for i in range(6):
        angle = math.radians(60 * i + 30)
        x = cx + HEX_SIZE * math.cos(angle)
        y = cy + HEX_SIZE * VERTICAL_SCALE * math.sin(angle)
        points.append((x, y))

    # 填充白色六边形
    draw.polygon(points, fill=(255, 255, 255, 255))

    path = os.path.join(OUTPUT_DIR, "T_HexOverlay.png")
    img.save(path)
    print(f"生成覆盖层贴图: {path} ({hex_width}x{hex_height})")
    print(f"  六边形顶点: {[(round(p[0],1), round(p[1],1)) for p in points]}")


def generate_hex_overlay_inner():
    """
    生成内缩六边形覆盖层：边长为原始的0.96，Y轴压扁到0.85
    用于移动范围等需要略小于格子的覆盖效果
    """
    inner_scale = 0.96
    inner_vertical = 0.85
    inner_size = HEX_SIZE * inner_scale

    # 使用原始六边形的画布尺寸，确保中心对齐
    hex_width = int(2 * HEX_SIZE * math.cos(math.radians(30))) + 4
    hex_height = int(2 * HEX_SIZE * VERTICAL_SCALE) + 4

    img = Image.new('RGBA', (hex_width, hex_height), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)

    cx = hex_width / 2
    cy = hex_height / 2

    # 计算内缩六边形顶点（平顶，30°偏移）
    points = []
    for i in range(6):
        angle = math.radians(60 * i + 30)
        x = cx + inner_size * math.cos(angle)
        y = cy + inner_size * inner_vertical * math.sin(angle)
        points.append((x, y))

    draw.polygon(points, fill=(255, 255, 255, 255))

    path = os.path.join(OUTPUT_DIR, "T_HexOverlayInner.png")
    img.save(path)
    print(f"生成内缩覆盖层贴图: {path} ({hex_width}x{hex_height})")
    print(f"  边长缩放: {inner_scale}, Y轴压扁: {inner_vertical}")
    print(f"  六边形顶点: {[(round(p[0],1), round(p[1],1)) for p in points]}")


def generate_transition_mask():
    """
    生成地形过渡遮罩纹理 T_TransitionMask.png

    灰度图，用于控制邻居地形在本格子上的渗透区域。
    默认朝右（东方向，0°），材质中通过 CustomRotator 旋转到 6 个方向。

    渐变逻辑（基于 UV 空间，中心 0.5,0.5）：
    - 从中心向右边缘，Alpha 从 0 渐变到 1
    - 左半部分全黑（不显示邻居纹理）
    - 渐变区域使用方向权重：越接近正右方向越强
    """
    size = 256
    img = Image.new('L', (size, size), 0)  # 灰度图

    cx, cy = size / 2, size / 2
    max_radius = size / 2

    # 过渡起始点（从中心到右边缘的多少比例处开始渐变）
    fade_start = 0.25  # 从 25% 半径处开始渐变
    # 角度衰减宽度（控制扇形的展开角度）
    angle_spread = math.radians(75)  # 每侧 75°，总覆盖 150° 扇形

    for y in range(size):
        for x in range(size):
            # 转换到 -1..1 的归一化坐标
            nx = (x - cx) / max_radius
            ny = (y - cy) / max_radius

            dist = math.sqrt(nx * nx + ny * ny)
            if dist < 0.001:
                img.putpixel((x, y), 0)
                continue

            # 到右侧边缘的方向权重（cos 角度，1=正右，0=垂直，-1=正左）
            angle = math.atan2(ny, nx)  # 相对于正右方向的角度
            abs_angle = abs(angle)

            # 角度权重：在扇形范围内从 1 衰减到 0
            if abs_angle > angle_spread:
                angle_weight = 0.0
            else:
                # 平滑衰减（使用 smoothstep）
                t = abs_angle / angle_spread
                angle_weight = 1.0 - (3 * t * t - 2 * t * t * t)  # smoothstep

            # 径向权重：从 fade_start 到 1.0 之间线性渐变
            if dist < fade_start:
                radial_weight = 0.0
            elif dist > 1.0:
                radial_weight = 1.0
            else:
                t = (dist - fade_start) / (1.0 - fade_start)
                # smoothstep 使渐变更柔和
                radial_weight = 3 * t * t - 2 * t * t * t

            # 合成最终值
            value = angle_weight * radial_weight

            # 添加少量噪声使边缘更自然
            noise = random.gauss(0, 0.02)
            value = max(0.0, min(1.0, value + noise))

            img.putpixel((x, y), int(value * 255))

    path = os.path.join(OUTPUT_DIR, "T_TransitionMask.png")
    img.save(path)
    print(f"生成过渡遮罩: {path} ({size}x{size})")
    print(f"  扇形角度: ±{math.degrees(angle_spread):.0f}°, 渐变起始: {fade_start*100:.0f}%")


if __name__ == "__main__":
    os.makedirs(OUTPUT_DIR, exist_ok=True)
    generate_edge_sprite()
    generate_hex_overlay()
    generate_hex_overlay_inner()
    generate_transition_mask()
    print("\n完成！接下来在 UE 编辑器中：")
    print("1. 导入这两张 PNG（拖入 Content Browser）")
    print("2. 右键 T_HexEdge → Create Sprite → 命名为 S_HexEdge")
    print("3. 右键 T_HexOverlay → Create Sprite → 命名为 S_HexOverlay")
    print("4. 右键 T_HexOverlayInner → Create Sprite → 命名为 S_HexOverlayInner")
    print("5. 右键 T_TransitionMask → 导入为纹理（用于 M_TerrainTransition 材质）")
    print("6. 在 GridManager 蓝图的 Highlight 分类中配置：")
    print("   - EdgeHighlightSprite = S_HexEdge")
    print("   - RangeOverlaySprite = S_HexOverlay 或 S_HexOverlayInner")
