/*
 * estimation_mode.cpp — Estimation mode management implementation
 */
#include "estimation_mode.h"

#include <cstdio>

namespace ble {

ModeManager::ModeManager()
    : mode_(EstimationMode::ACCURACY)
    , low_conf_streak_(0)
    , high_conf_streak_(0)
{
    /* Accuracy mode: more samples, slower, smoothed */
    accuracy_params_.samples_per_scan     = ACCURACY_SAMPLES;
    accuracy_params_.scan_interval_ms     = ACCURACY_SCAN_INTERVAL_MS;
    accuracy_params_.smoothing_alpha      = ACCURACY_SMOOTHING_ALPHA;
    accuracy_params_.confidence_threshold = ACCURACY_CONF_THRESHOLD;
    accuracy_params_.display_name         = "ACCURACY";

    /* Responsive mode: fewer samples, faster, raw */
    responsive_params_.samples_per_scan     = RESPONSIVE_SAMPLES;
    responsive_params_.scan_interval_ms     = RESPONSIVE_SCAN_INTERVAL_MS;
    responsive_params_.smoothing_alpha      = RESPONSIVE_SMOOTHING_ALPHA;
    responsive_params_.confidence_threshold = RESPONSIVE_CONF_THRESHOLD;
    responsive_params_.display_name         = "RESPONSIVE";
}

void ModeManager::set_mode(EstimationMode mode)
{
    if (mode_ != mode) {
        mode_ = mode;
        low_conf_streak_  = 0;
        high_conf_streak_ = 0;
        std::printf("  [MODE] Switched to %s mode\n", params().display_name);
    }
}

void ModeManager::toggle()
{
    if (mode_ == EstimationMode::ACCURACY)
        set_mode(EstimationMode::RESPONSIVE);
    else
        set_mode(EstimationMode::ACCURACY);
}

const ModeParams &ModeManager::params() const
{
    return params_for(mode_);
}

const ModeParams &ModeManager::params_for(EstimationMode mode) const
{
    if (mode == EstimationMode::ACCURACY) return accuracy_params_;
    return responsive_params_;
}

void ModeManager::print_status() const
{
    const auto &p = params();
    std::printf("  Mode: %s  (samples=%d, interval=%ums, "
                "smoothing=%.2f, conf_thresh=%.2f)\n",
                p.display_name, p.samples_per_scan,
                p.scan_interval_ms, p.smoothing_alpha,
                p.confidence_threshold);
}

bool ModeManager::auto_adjust(float confidence)
{
    const auto &p = params();

    if (confidence < p.confidence_threshold) {
        low_conf_streak_++;
        high_conf_streak_ = 0;
    } else {
        high_conf_streak_++;
        low_conf_streak_ = 0;
    }

    /* If in RESPONSIVE mode and confidence is consistently low,
       switch to ACCURACY for better results */
    if (mode_ == EstimationMode::RESPONSIVE &&
        low_conf_streak_ >= AUTO_SWITCH_STREAK) {
        set_mode(EstimationMode::ACCURACY);
        return true;
    }

    /* If in ACCURACY mode and confidence is consistently high,
       switch to RESPONSIVE for faster updates */
    if (mode_ == EstimationMode::ACCURACY &&
        high_conf_streak_ >= AUTO_SWITCH_STREAK) {
        set_mode(EstimationMode::RESPONSIVE);
        return true;
    }

    return false;
}

} // namespace ble
