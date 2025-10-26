
#include "password_strength.h"
#include <cmath>
#include <cctype>
#include <algorithm>
#include <stdexcept>
#include <limits>
#include <sstream>
#include <iomanip>

double calculate_pool_size(const std::string &password) {
    bool has_lower = false;
    bool has_upper = false;
    bool has_digit = false;
    bool has_symbol = false;

    for (char c : password) {
        if (std::islower(c)) has_lower = true;
        else if (std::isupper(c)) has_upper = true;
        else if (std::isdigit(c)) has_digit = true;
        else has_symbol = true;
    }

    double pool=0.0;
    if (has_lower) pool+=26.0;
    if (has_upper) pool+=26.0;
    if (has_digit) pool+=10.0;
    if (has_symbol) pool+=32.0;

    return pool;
}

double calculate_entropy(int length, double pool_size) {
    if (pool_size<=1) return 0.0; //0 if input is invalid
    return length*std::log2(pool_size);
}

double calculate_num_combinations(double entropy_bits) {
    return std::pow(2.0,entropy_bits);
}

CrackTimes estimate_crack_times(double num_combinations) {
    CrackTimes times;
    // sanity: if num_combinations is inf or nan -> return inf
    if (!std::isfinite(num_combinations) || num_combinations <= 0.0) {
        times.online_throttled = std::numeric_limits<double>::infinity();
        times.online_unthrottled = std::numeric_limits<double>::infinity();
        times.offline_slow = std::numeric_limits<double>::infinity();
        times.offline_fast = std::numeric_limits<double>::infinity();
        return times;
    }

    const double r_online_throttled = 100.0 / 3600.0; // 100 per hour -> ~0.02778 /s
    const double r_online_unthrottled = 10.0;         // 10 / s
    const double r_offline_slow = 1e4;                // 10k / s
    const double r_offline_fast = 1e10;               // 1e10 / s

    //if overflow -> inf
    auto safe_div = [](double a, double b)->double {
        if (b == 0.0) return std::numeric_limits<double>::infinity();
        double v = a / b;
        if (!std::isfinite(v)) return std::numeric_limits<double>::infinity();
        return v;
    };

    times.online_throttled   = safe_div(num_combinations, r_online_throttled);
    times.online_unthrottled = safe_div(num_combinations, r_online_unthrottled);
    times.offline_slow       = safe_div(num_combinations, r_offline_slow);
    times.offline_fast       = safe_div(num_combinations, r_offline_fast);
    return times;
}

std::string determine_strength_category(double entropy_bits) {

    if (entropy_bits < 28.0) return "Poor";
    if (entropy_bits < 36.0) return "Average";
    if (entropy_bits < 60.0) return "Strong";
    return "Very strong";
}

PasswordStrengthInfo analyze_password(const std::string &password) {
    PasswordStrengthInfo info;
    info.length = static_cast<int>(password.size());
    info.pool_size = calculate_pool_size(password);
    info.entropy_bits = calculate_entropy(info.length, info.pool_size);


    double num_comb = std::exp2(info.entropy_bits); // 2^entropy_bits
    if (!std::isfinite(num_comb) || num_comb < 1.0) {
        // if overflow or incorrect -> inf
        info.num_combinations = std::numeric_limits<double>::infinity();
    } else {
        info.num_combinations = num_comb;
    }

    CrackTimes times = estimate_crack_times(info.num_combinations);
    info.time_online_throttled = times.online_throttled;
    info.time_online_unthrottled = times.online_unthrottled;
    info.time_offline_slow = times.offline_slow;
    info.time_offline_fast = times.offline_fast;

    info.strength_category = determine_strength_category(info.entropy_bits);
    return info;
}


