"""
生成六边形橙红色发光边框 PNG。
纯 Python 实现，无外部依赖。
"""
import zlib
import struct
import math
import os

OUTPUT = os.path.join(os.path.dirname(__file__), "..", "Content", "Art", "HexSelectionGlow.png")
SIZE = 256          # 图像像素宽度（正方形）
HEX_RADIUS = 110    # 六边形外接圆半径（像素）
BORDER_WIDTH = 6    # 边框线宽（像素）
GLOW_WIDTH = 14     # 发光扩散宽度（像素）

# 颜色：橙红色系
GLOW_COLOR = (255, 80, 20)       # 橙红色发光
BORDER_COLOR = (255, 140, 60)    # 边框亮橙色

def hex_corner(center_x, center_y, radius, i):
    """尖顶六边形的第 i 个顶点（0-5），角度从 90° 开始"""
    angle_deg = 60 * i + 90
    angle_rad = math.radians(angle_deg)
    x = center_x + radius * math.cos(angle_rad)
    y = center_y + radius * math.sin(angle_rad)
    return x, y

def point_to_segment_dist(px, py, ax, ay, bx, by):
    """点到线段的最短距离"""
    dx, dy = bx - ax, by - ay
    if dx == 0 and dy == 0:
        return math.sqrt((px - ax) ** 2 + (py - ay) ** 2)
    t = max(0, min(1, ((px - ax) * dx + (py - ay) * dy) / (dx * dx + dy * dy)))
    cx, cy = ax + t * dx, ay + t * dy
    return math.sqrt((px - cx) ** 2 + (py - cy) ** 2)

def dist_to_hex_edge(px, py, cx, cy, radius):
    """像素到六边形边缘的最短距离（正值=外部，负值=内部）"""
    # 先判断是否在六边形外接圆内（快速剔除）
    dist_to_center = math.sqrt((px - cx) ** 2 + (py - cy) ** 2)
    if dist_to_center > radius + GLOW_WIDTH + BORDER_WIDTH:
        return 999.0

    # 计算到每条边的最短距离
    min_dist = float('inf')
    inside = True
    for i in range(6):
        x1, y1 = hex_corner(cx, cy, radius, i)
        x2, y2 = hex_corner(cx, cy, radius, (i + 1) % 6)
        d = point_to_segment_dist(px, py, x1, y1, x2, y2)
        min_dist = min(min_dist, d)

        # 判断点是否在边的内侧（用叉积）
        edge_x, edge_y = x2 - x1, y2 - y1
        to_px, to_py = px - x1, py - y1
        cross = edge_x * to_py - edge_y * to_px
        if cross > 0:
            inside = False

    # 内部距离为负值
    return -min_dist if inside else min_dist

def generate():
    cx = cy = SIZE / 2.0
    pixels = []

    for y in range(SIZE):
        row = []
        for x in range(SIZE):
            d = dist_to_hex_edge(x, y, cx, cy, HEX_RADIUS)
            r, g, b, a = 0, 0, 0, 0

            if d < 0:
                # 内部：无填充，保持透明
                pass
            elif d < BORDER_WIDTH:
                # 边框线
                t = d / BORDER_WIDTH
                alpha = int(255 * (1.0 - t * 0.3))
                r, g, b, a = BORDER_COLOR[0], BORDER_COLOR[1], BORDER_COLOR[2], alpha
            elif d < BORDER_WIDTH + GLOW_WIDTH:
                # 发光区域
                t = (d - BORDER_WIDTH) / GLOW_WIDTH
                alpha = int(255 * (1.0 - t) ** 2)  # 二次衰减
                r, g, b, a = GLOW_COLOR[0], GLOW_COLOR[1], GLOW_COLOR[2], alpha

            row.append((r, g, b, a))
        pixels.append(row)

    # 写入 PNG
    write_png(pixels, SIZE, SIZE)

def write_png(pixels, width, height):
    """纯 Python PNG 编码器"""
    # 构建原始像素数据（RGBA，逐行）
    raw_data = b''
    for y in range(height):
        raw_data += b'\x00'  # 过滤器：None
        for x in range(width):
            r, g, b, a = pixels[y][x]
            raw_data += struct.pack('BBBB', r, g, b, a)

    # PNG 签名
    signature = b'\x89PNG\r\n\x1a\n'

    # IHDR
    ihdr_data = struct.pack('>IIBBBBB', width, height, 8, 6, 0, 0, 0)  # 8-bit RGBA
    ihdr = make_chunk(b'IHDR', ihdr_data)

    # IDAT
    compressed = zlib.compress(raw_data)
    idat = make_chunk(b'IDAT', compressed)

    # IEND
    iend = make_chunk(b'IEND', b'')

    os.makedirs(os.path.dirname(OUTPUT), exist_ok=True)
    with open(OUTPUT, 'wb') as f:
        f.write(signature)
        f.write(ihdr)
        f.write(idat)
        f.write(iend)

    print(f"Generated: {OUTPUT}  ({width}x{height})")

def make_chunk(chunk_type, data):
    chunk = chunk_type + data
    crc = struct.pack('>I', zlib.crc32(chunk) & 0xFFFFFFFF)
    length = struct.pack('>I', len(data))
    return length + chunk + crc

if __name__ == '__main__':
    generate()
