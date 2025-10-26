// password_strength.h
#ifndef PASSWORD_STRENGTH_H
#define PASSWORD_STRENGTH_H

#include <string>

//structures
struct CrackTimes {};
struct PasswordStrengthInfo {};

//function declarations
std::string human_time(double seconds);
double calculate_pool_size(const std::string &password);
double calculate_entropy(int length, double pool_size);
double calculate_num_combinations(double entropy_bits);
CrackTimes estimate_crack_times(double num_combinations);
std::string determine_strength_category(double entropy_bits);
PasswordStrengthInfo analyze_password(const std::string &password);

#endif
