#ifndef PASSWORD_STRENGTH_H
#define PASSWORD_STRENGTH_H

#include <string>

struct CrackTimes {
    double online_throttled;
    double online_unthrottled;
    double offline_slow;
    double offline_fast;
};

struct PasswordStrengthInfo {
    int length;
    double pool_size;
    double entropy_bits;
    double num_combinations;
    double time_online_throttled;
    double time_online_unthrottled;
    double time_offline_slow;
    double time_offline_fast;
    std::string strength_category;
};

//functions for CrackTimes
double calculate_pool_size(const std::string &password);
double calculate_entropy(int length, double pool_size);
double calculate_num_combinations(double entropy_bits);
CrackTimes estimate_crack_times(double num_combinations);
std::string determine_strength_category(double entropy_bits);

//functions for PasswordStrengthInfo
PasswordStrengthInfo analyze_password(const std::string &password);



#endif //PASSWORD_STRENGTH_H
