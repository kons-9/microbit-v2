/*
 * location.h — Position estimation result management
 *
 * Provides:
 *   - Position recording with optional EMA smoothing
 *   - Area-based dwell time tracking (zone grid)
 *   - Movement speed calculation
 *   - ASCII room map visualization
 */
#pragma once

#include "app_config.h"

#include <cstdint>

namespace ble {

struct Position {
    float    x;
    float    y;
    float    confidence;
    uint32_t timestamp_ms;
};

class LocationTracker {
public:
    LocationTracker() = default;

    /* Record a new estimated position.
       smoothing_alpha: 0.0=ignore new (use previous), 1.0=use raw new value.
       Typical: 0.3 for smoothing, 1.0 for no smoothing. */
    void record(float x, float y, float confidence, uint32_t tick,
                float smoothing_alpha = 1.0f);

    /* Get latest position */
    const Position &latest() const;

    /* Get number of recorded positions */
    int count() const { return count_; }

    /* Get movement speed (m/s) between last two positions.
       Returns 0.0 if fewer than 2 positions recorded. */
    float speed() const;

    /* Get total distance traveled (meters) */
    float total_distance() const { return total_dist_; }

    /* Print ASCII map of room with position history */
    void print_map() const;

    /* Print ASCII map with beacon positions overlaid */
    void print_map_with_beacons(const float *bx, const float *by,
                                const char (*names)[16], int n_beacons) const;

    /* Print position history table */
    void print_history(int n = 5) const;

    /* --- Area / zone dwell time tracking --- */

    /* Update zone dwell time based on latest position.
       Call after record(). delta_ms = time since last update. */
    void update_zone(uint32_t delta_ms);

    /* Print zone dwell time heatmap */
    void print_zones() const;

    /* Get dwell time (ms) for a specific zone */
    uint32_t zone_dwell(int col, int row) const;

private:
    Position history_[POSITION_HISTORY]{};
    int      count_{0};
    int      write_idx_{0};
    float    total_dist_{0.0f};

    /* Zone grid dwell time (ms) */
    uint32_t zone_time_[ZONE_ROWS][ZONE_COLS]{};
};

} // namespace ble
