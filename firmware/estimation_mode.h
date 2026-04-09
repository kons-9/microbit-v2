/*
 * estimation_mode.h — Estimation mode management
 *
 * Provides two estimation modes:
 *   ACCURACY     — more RSSI samples, position smoothing, slower but precise
 *   RESPONSIVE   — fewer samples, no smoothing, faster updates
 *
 * Each mode has its own set of scan/inference parameters.
 * Mode can be toggled at runtime.
 */
#pragma once

#include "app_config.h"

#include <cstdint>

namespace ble {

enum class EstimationMode : uint8_t {
    ACCURACY   = 0,
    RESPONSIVE = 1,
};

struct ModeParams {
    int      samples_per_scan;       /* RSSI samples per beacon per scan */
    uint32_t scan_interval_ms;       /* interval between scans */
    float    smoothing_alpha;        /* EMA smoothing (0.0=no, 1.0=raw) */
    float    confidence_threshold;   /* minimum confidence to accept */
    const char *display_name;
};

class ModeManager {
public:
    ModeManager();

    /* Switch to a specific mode */
    void set_mode(EstimationMode mode);

    /* Toggle between ACCURACY and RESPONSIVE */
    void toggle();

    /* Current mode */
    EstimationMode mode() const { return mode_; }

    /* Parameters for the current mode */
    const ModeParams &params() const;

    /* Parameters for a specific mode */
    const ModeParams &params_for(EstimationMode mode) const;

    /* Print current mode info */
    void print_status() const;

    /* Auto-switch logic: if confidence consistently low, switch to ACCURACY;
       if consistently high, allow RESPONSIVE.
       Returns true if a switch occurred. */
    bool auto_adjust(float confidence);

private:
    EstimationMode mode_;
    ModeParams     accuracy_params_;
    ModeParams     responsive_params_;

    /* Auto-adjust state */
    int   low_conf_streak_;
    int   high_conf_streak_;
};

} // namespace ble
