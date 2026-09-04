#!/bin/bash
# ============================================================
# pull_photos.sh — 拉取 YUV → 自动分组转 JPG，续号
# ============================================================
# 远端文件: gesture_1_001.yuv … gesture_5_100.yuv
# 本地:
#   /mnt/c/.../gesture/gesture_1_images/gesture_1_001.jpg …
#   /mnt/c/.../gesture/gesture_5_images/gesture_5_001.jpg …
# 自动按前缀分组，根据已有文件续号
#
# 用法:
#   ./pull_photos.sh                  # 拉取 + 分组转换
#   ./pull_photos.sh -c               # 拉取 + 转换后清理远端
#   ./pull_photos.sh -r               # 仅转换本地已有的 YUV
#   ./pull_photos.sh -s 1920x1080     # 指定分辨率 (默认 1920x1080)
#   ./pull_photos.sh -f nv21          # 指定像素格式 (默认 nv21)

set -euo pipefail

# ── 配置 ────────────────────────────────────────────────────
DUO_IP="${DUO_IP:-192.168.42.1}"
DUO_USER="${DUO_USER:-root}"
REMOTE_DIR="/root/photo"
GESTURE_BASE="/mnt/c/Users/Administrator/Desktop/gesture"
TEMP_DIR="/tmp/photo_pull_$$"

WIDTH=1920
HEIGHT=1080
PIX_FMT="nv21"

CLEAN_REMOTE=0
LOCAL_ONLY=0

# ── 参数解析 ────────────────────────────────────────────────
while [[ $# -gt 0 ]]; do
    case "$1" in
    -c | --clean) CLEAN_REMOTE=1 ;;
    -r | --local) LOCAL_ONLY=1 ;;
    -s | --size)
        WIDTH="${2%x*}"
        HEIGHT="${2#*x}"
        shift
        ;;
    -f | --format)
        PIX_FMT="$2"
        shift
        ;;
    -h | --help)
        echo "用法: $0 [选项]"
        echo "  -c, --clean       拉取后删除远端文件"
        echo "  -r, --local       仅转换本地已有的 YUV (不拉取)"
        echo "  -s, --size WxH    分辨率 (默认 1920x1080)"
        echo "  -f, --format F    像素格式 (默认 nv21)"
        echo "  -h, --help        显示帮助"
        echo ""
        echo "自动按 gesture_X 前缀分组到 gesture_X_images/ 目录，续号"
        exit 0
        ;;
    *)
        echo "未知选项: $1"
        exit 1
        ;;
    esac
    shift
done

# ── 检查依赖 ────────────────────────────────────────────────
check_dep() {
    if ! command -v "$1" &>/dev/null; then
        echo "[ERROR] 缺少依赖: $1，请先安装"
        exit 1
    fi
}

check_dep ffmpeg
if [[ $LOCAL_ONLY -eq 0 ]]; then
    check_dep scp
fi

mkdir -p "$TEMP_DIR"

# ── 拉取远端文件 ────────────────────────────────────────────
if [[ $LOCAL_ONLY -eq 0 ]]; then
    echo "═══════════════════════════════════════════════════════"
    echo "  拉取远端: ${DUO_USER}@${DUO_IP}:${REMOTE_DIR}"
    echo "═══════════════════════════════════════════════════════"

    FILE_COUNT=$(ssh "${DUO_USER}@${DUO_IP}" \
        "ls ${REMOTE_DIR}/*.yuv 2>/dev/null | wc -l" 2>/dev/null || echo "0")
    FILE_COUNT=$(echo "$FILE_COUNT" | tr -d ' ')

    if [[ "$FILE_COUNT" -eq 0 ]]; then
        echo "[INFO] 远端无 .yuv 文件，跳过拉取"
    else
        echo "[INFO] 发现 ${FILE_COUNT} 个 YUV 文件，开始拉取..."
        scp "${DUO_USER}@${DUO_IP}:${REMOTE_DIR}"/*.yuv "$TEMP_DIR/" 2>&1
        echo "[OK] 拉取完成"

        if [[ $CLEAN_REMOTE -eq 1 ]]; then
            echo "[INFO] 清理远端..."
            ssh "${DUO_USER}@${DUO_IP}" "rm -f ${REMOTE_DIR}/*.yuv"
            echo "[OK] 远端已清理"
        fi
    fi
fi

# ── 收集 YUV 文件 ───────────────────────────────────────────
mapfile -t YUV_FILES < <(ls "$TEMP_DIR"/*.yuv 2>/dev/null | sort)

if [[ ${#YUV_FILES[@]} -eq 0 ]]; then
    echo "[INFO] 无 YUV 文件需要转换"
    rm -rf "$TEMP_DIR"
    exit 0
fi

# ── 按前缀分组 ──────────────────────────────────────────────
# gesture_1_042.yuv → prefix=gesture_1
declare -A GROUP_MAP         # prefix → "file1 file2 ..." (空格分隔)
declare -a PREFIXES=()      # 保持顺序

for yuv in "${YUV_FILES[@]}"; do
    base=$(basename "$yuv" .yuv)
    # 去掉末尾 _NNN 得到前缀: gesture_1_042 → gesture_1
    prefix="${base%_[0-9][0-9][0-9]}"
    if [[ -z "$prefix" || "$prefix" == "$base" ]]; then
        echo "[WARN] 无法解析前缀: $base，跳过"
        continue
    fi
    if [[ -z "${GROUP_MAP[$prefix]:-}" ]]; then
        PREFIXES+=("$prefix")
        GROUP_MAP[$prefix]="$yuv"
    else
        GROUP_MAP[$prefix]="${GROUP_MAP[$prefix]} $yuv"
    fi
done

if [[ ${#PREFIXES[@]} -eq 0 ]]; then
    echo "[INFO] 无可识别的分组"
    rm -rf "$TEMP_DIR"
    exit 0
fi

# ── 逐组转换 ────────────────────────────────────────────────
TOTAL_CONVERTED=0
TOTAL_FAILED=0

echo ""
echo "═══════════════════════════════════════════════════════"
echo "  分组转换  YUV → JPG   (${WIDTH}x${HEIGHT} ${PIX_FMT})"
echo "═══════════════════════════════════════════════════════"

for prefix in "${PREFIXES[@]}"; do
    TARGET_DIR="${GESTURE_BASE}/${prefix}_images"
    mkdir -p "$TARGET_DIR"

    # 查找该目录下已有最大编号
    max_num=0
    for jpg in "$TARGET_DIR"/"${prefix}"_*.jpg; do
        [[ -f "$jpg" ]] || continue
        name=$(basename "$jpg" .jpg)
        num_str="${name##*_}"
        if [[ "$num_str" =~ ^[0-9]+$ ]]; then
            num=$((10#$num_str))
            [[ $num -gt $max_num ]] && max_num=$num
        fi
    done

    seq=$((max_num + 1))
    converted=0
    failed=0

    echo ""
    echo "── ${prefix} → ${prefix}_images/  (续号: ${seq}) ──"

    for yuv in ${GROUP_MAP[$prefix]}; do
        [[ ! -f "$yuv" ]] && continue

        seq_str=$(printf '%03d' $seq)
        output="${TARGET_DIR}/${prefix}_${seq_str}.jpg"

        echo -n "  ${prefix}_${seq_str}.jpg ... "

        if ffmpeg -hide_banner -loglevel error \
            -f rawvideo \
            -pix_fmt "$PIX_FMT" \
            -s "${WIDTH}x${HEIGHT}" \
            -i "$yuv" \
            -frames:v 1 \
            "$output" 2>&1; then
            echo "OK"
            converted=$((converted + 1))
        else
            echo "FAIL"
            failed=$((failed + 1))
        fi
        seq=$((seq + 1))
    done

    echo "  → ${converted} OK, ${failed} FAIL"
    TOTAL_CONVERTED=$((TOTAL_CONVERTED + converted))
    TOTAL_FAILED=$((TOTAL_FAILED + failed))
done

# ── 清理 ────────────────────────────────────────────────────
rm -rf "$TEMP_DIR"

# ── 汇总 ────────────────────────────────────────────────────
echo ""
echo "═══════════════════════════════════════════════════════"
echo "  总计: ${TOTAL_CONVERTED} 张成功, ${TOTAL_FAILED} 张失败"
echo "  位置: ${GESTURE_BASE}/"
echo "═══════════════════════════════════════════════════════"

for prefix in "${PREFIXES[@]}"; do
    dir="${GESTURE_BASE}/${prefix}_images"
    count=$(ls "$dir"/*.jpg 2>/dev/null | wc -l)
    echo "  ${prefix}_images/ : ${count} 张"
done
