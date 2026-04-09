/*
 * location.cpp — LocationTracker implementation
 */
#include "location.h"

#include <cstdio>
#include <cstring>
#include <cmath>

namespace ble {

void LocationTracker::record(float x, float y, float confidence,
                             uint32_t tick, float smoothing_alpha)
{
    /* Apply exponential moving average smoothing if we have a previous pos */
    if (count_ > 0 && smoothing_alpha < 1.0f) {
        const auto &prev = latest();
        x = smoothing_alpha * x + (1.0f - smoothing_alpha) * prev.x;
        y = smoothing_alpha * y + (1.0f - smoothing_alpha) * prev.y;
    }

    /* Accumulate distance traveled */
    if (count_ > 0) {
        const auto &prev = latest();
        float dx = x - prev.x;
        float dy = y - prev.y;
        total_dist_ += std::sqrt(dx * dx + dy * dy);
    }

    auto &p = history_[write_idx_];
    p.x            = x;
    p.y            = y;
    p.confidence   = confidence;
    p.timestamp_ms = tick;

    write_idx_ = (write_idx_ + 1) % POSITION_HISTORY;
    if (count_ < POSITION_HISTORY) count_++;
}

const Position &LocationTracker::latest() const
{
    int idx = (write_idx_ - 1 + POSITION_HISTORY) % POSITION_HISTORY;
    return history_[idx];
}

float LocationTracker::speed() const
{
    if (count_ < 2) return 0.0f;

    int cur_idx  = (write_idx_ - 1 + POSITION_HISTORY) % POSITION_HISTORY;
    int prev_idx = (write_idx_ - 2 + POSITION_HISTORY) % POSITION_HISTORY;
    const auto &cur  = history_[cur_idx];
    const auto &prev = history_[prev_idx];

    float dx = cur.x - prev.x;
    float dy = cur.y - prev.y;
    float dist = std::sqrt(dx * dx + dy * dy);

    uint32_t dt_ms = cur.timestamp_ms - prev.timestamp_ms;
    if (dt_ms == 0) return 0.0f;

    return dist / (static_cast<float>(dt_ms) / 1000.0f);
}

/* --- Visualization --- */

void LocationTracker::print_map() const
{
    print_map_with_beacons(nullptr, nullptr, nullptr, 0);
}

void LocationTracker::print_map_with_beacons(
    const float *bx, const float *by,
    const char (*names)[16], int n_beacons) const
{
    /* ASCII room map: 40 x 20 characters */
    static constexpr int MAP_W = 40;
    static constexpr int MAP_H = 20;
    char map[MAP_H][MAP_W + 1];

    /* Draw room border */
    for (int y = 0; y < MAP_H; y++) {
        for (int x = 0; x < MAP_W; x++) {
            if (y == 0 || y == MAP_H - 1)
                map[y][x] = '-';
            else if (x == 0 || x == MAP_W - 1)
                map[y][x] = '|';
            else
                map[y][x] = ' ';
        }
        map[y][MAP_W] = '\0';
    }

    /* Plot beacon positions as [A], [B], ... */
    if (bx && by && n_beacons > 0) {
        for (int i = 0; i < n_beacons; i++) {
            int mx = 1 + static_cast<int>((bx[i] / ROOM_W) * (MAP_W - 2));
            int my = 1 + static_cast<int>((by[i] / ROOM_H) * (MAP_H - 2));
            if (mx < 1) mx = 1;
            if (mx >= MAP_W - 1) mx = MAP_W - 2;
            if (my < 1) my = 1;
            if (my >= MAP_H - 1) my = MAP_H - 2;
            char label = (names && names[i][0] != '\0')
                ? names[i][std::strlen(names[i]) - 1]  /* last char: e.g. 'A' */
                : static_cast<char>('1' + i);
            map[my][mx] = label;
        }
    }

    /* Plot position history (older = '.', newer = 'o', latest = '*') */
    for (int i = 0; i < count_; i++) {
        int real_idx = (write_idx_ - count_ + i + POSITION_HISTORY)
                       % POSITION_HISTORY;
        const auto &p = history_[real_idx];

        int mx = 1 + static_cast<int>((p.x / ROOM_W) * (MAP_W - 2));
        int my = 1 + static_cast<int>((p.y / ROOM_H) * (MAP_H - 2));
        if (mx < 1) mx = 1;
        if (mx >= MAP_W - 1) mx = MAP_W - 2;
        if (my < 1) my = 1;
        if (my >= MAP_H - 1) my = MAP_H - 2;

        if (i == count_ - 1)
            map[my][mx] = '*';  /* latest */
        else if (i >= count_ - 3)
            map[my][mx] = 'o';  /* recent */
        else
            map[my][mx] = '.';  /* older */
    }

    std::printf("  Room Map (%.0fm x %.0fm):\n", ROOM_W, ROOM_H);
    for (int y = 0; y < MAP_H; y++)
        std::printf("  %s\n", map[y]);
}

void LocationTracker::print_history(int n) const
{
    std::printf("  %-6s %-8s %-8s %-10s %-10s %s\n",
                "#", "X(m)", "Y(m)", "Conf", "Speed", "Time(ms)");
    int start = (count_ > n) ? count_ - n : 0;
    for (int i = start; i < count_; i++) {
        int real_idx = (write_idx_ - count_ + i + POSITION_HISTORY)
                       % POSITION_HISTORY;
        const auto &p = history_[real_idx];

        /* Calculate per-step speed */
        float spd = 0.0f;
        if (i > 0) {
            int prev_idx = (write_idx_ - count_ + i - 1 + POSITION_HISTORY)
                           % POSITION_HISTORY;
            const auto &prev = history_[prev_idx];
            float dx = p.x - prev.x;
            float dy = p.y - prev.y;
            float dist = std::sqrt(dx * dx + dy * dy);
            uint32_t dt = p.timestamp_ms - prev.timestamp_ms;
            if (dt > 0) spd = dist / (static_cast<float>(dt) / 1000.0f);
        }

        std::printf("  %-6d %-8.2f %-8.2f %-10.3f %-10.2f %u\n",
                    i, p.x, p.y, p.confidence, spd, p.timestamp_ms);
    }
    std::printf("  Total distance: %.2f m\n", total_dist_);
}

/* --- Zone dwell time --- */

void LocationTracker::update_zone(uint32_t delta_ms)
{
    if (count_ == 0) return;

    const auto &p = latest();

    /* Map position to zone grid */
    int col = static_cast<int>((p.x / ROOM_W) * ZONE_COLS);
    int row = static_cast<int>((p.y / ROOM_H) * ZONE_ROWS);
    if (col < 0) col = 0;
    if (col >= ZONE_COLS) col = ZONE_COLS - 1;
    if (row < 0) row = 0;
    if (row >= ZONE_ROWS) row = ZONE_ROWS - 1;

    zone_time_[row][col] += delta_ms;
}

uint32_t LocationTracker::zone_dwell(int col, int row) const
{
    if (col < 0 || col >= ZONE_COLS || row < 0 || row >= ZONE_ROWS)
        return 0;
    return zone_time_[row][col];
}

void LocationTracker::print_zones() const
{
    /* Find max dwell time for scaling the heatmap */
    uint32_t max_time = 0;
    for (int r = 0; r < ZONE_ROWS; r++)
        for (int c = 0; c < ZONE_COLS; c++)
            if (zone_time_[r][c] > max_time)
                max_time = zone_time_[r][c];

    std::printf("  Zone Dwell Time (%.0fm x %.0fm, %dx%d grid):\n",
                ROOM_W, ROOM_H, ZONE_COLS, ZONE_ROWS);

    /* Column header */
    std::printf("  ");
    for (int c = 0; c < ZONE_COLS; c++)
        std::printf("  %-8s", "--------");
    std::printf("\n");

    /* Heatmap characters by intensity */
    static const char heat[] = " .:;+=xX#@";
    static constexpr int HEAT_LEN = 10;

    for (int r = 0; r < ZONE_ROWS; r++) {
        std::printf("  |");
        for (int c = 0; c < ZONE_COLS; c++) {
            float frac = (max_time > 0)
                ? static_cast<float>(zone_time_[r][c]) /
                  static_cast<float>(max_time)
                : 0.0f;
            int level = static_cast<int>(frac * (HEAT_LEN - 1));
            if (level >= HEAT_LEN) level = HEAT_LEN - 1;
            char ch = heat[level];

            float secs = static_cast<float>(zone_time_[r][c]) / 1000.0f;
            std::printf(" %c %5.1fs |", ch, secs);
        }
        std::printf("\n");
    }

    std::printf("  ");
    for (int c = 0; c < ZONE_COLS; c++)
        std::printf("  %-8s", "--------");
    std::printf("\n");
}

} // namespace ble
