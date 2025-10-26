//
// Created by kubaz on 26.10.2025.
//

#include "password_generator.h"
#include <random>
#include <algorithm>
#include <vector>
#include <stdexcept>

static const std::string LOWER = "abcdefghijklmnopqrstuvwxyz";
static const std::string UPPER = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
static const std::string DIGITS = "0123456789";
static const std::string SYMBOLS = "!@#$%^&*()-_=+[]{};:,.<>?/";

std::string generate_strong_password(int length) {
    if (length <= 0) throw std::invalid_argument("length must be > 0");

    //rng
    std::random_device rd;
    std::mt19937_64 gen(rd());

    const std::string all = LOWER + UPPER + DIGITS + SYMBOLS;
    std::uniform_int_distribution<size_t> dist_all(0, all.size() - 1);
    std::uniform_int_distribution<size_t> dist_lower(0, LOWER.size() - 1);
    std::uniform_int_distribution<size_t> dist_upper(0, UPPER.size() - 1);
    std::uniform_int_distribution<size_t> dist_digits(0, DIGITS.size() - 1);
    std::uniform_int_distribution<size_t> dist_symbols(0, SYMBOLS.size() - 1);

    std::vector<char> pw;
    pw.reserve(length);

    if (length >= 4) {
        //at least one character from each category
        pw.push_back(LOWER[dist_lower(gen)]);
        pw.push_back(UPPER[dist_upper(gen)]);
        pw.push_back(DIGITS[dist_digits(gen)]);
        pw.push_back(SYMBOLS[dist_symbols(gen)]);
        for (int i = 4; i < length; ++i) {
            pw.push_back(all[dist_all(gen)]);
        }
    } else {
        for (int i = 0; i < length; ++i) {
            pw.push_back(all[dist_all(gen)]);
        }
    }

    std::shuffle(pw.begin(), pw.end(), gen);
    return std::string(pw.begin(), pw.end());
}
