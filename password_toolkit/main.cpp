#include <iostream>
#include <iomanip>
#include <limits>
#include <cmath>
#include <sstream>
#include <string>
#include "password_strength.h"
#include "password_generator.h"

using namespace std;

//function that helps people read timestamps
string human_time(double seconds) {
    if (!isfinite(seconds)) return "Infinity";
    if (seconds < 1.0) return "less than 1 s";

    const double minute = 60.0;
    const double hour = 3600.0;
    const double day = 86400.0;
    const double year = 31557600.0;

    ostringstream oss;
    oss << fixed << setprecision(2);

    if (seconds < minute) {
        oss << seconds << " s";
    } else if (seconds < hour) {
        oss << (seconds / minute) << " min";
    } else if (seconds < day) {
        oss << (seconds / hour) << " hour";
    } else if (seconds < year) {
        oss << (seconds / day) << " days";
    } else {
        oss << (seconds / year) << " years";
    }
    return oss.str();
}

int main() {
    while (true) {
        cout << "=== Password Toolkit ===\n";
        cout << "1) Check password strength\n";
        cout << "2) Generate strong password\n";
        cout << "0) Exit\n";
        cout << "Enter your choice: ";

        int choice;
        if (!(cin >> choice)) {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Invalid input! Try again.\n";
            continue;
        }

        cin.ignore(numeric_limits<streamsize>::max(), '\n'); //cleaning

        if (choice == 0) {
            cout << "Goodbye!\n";
            break;
        }

        if (choice == 1) {
            string password;
            cout << "Enter your password: ";
            getline(cin, password);

            PasswordStrengthInfo info = analyze_password(password);

            cout << "\nPassword: " << password << "\n";
            cout << "Length: " << info.length << "\n";
            cout << "Pool size: " << info.pool_size << "\n";
            cout << "Entropy (bits): " << info.entropy_bits << "\n";
            cout << "Category: " << info.strength_category << "\n";
            cout << "Time to crack (offline fast): " << human_time(info.time_offline_fast) << "\n";

        } else if (choice == 2) {
            int length;
            cout << "Enter password length: ";
            if (!(cin >> length)) {
                cin.clear();
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                cout << "Invalid input!\n";
                continue;
            }

            try {
                string password = generate_strong_password(length);
                cout << "\nGenerated password: " << password << "\n";

                PasswordStrengthInfo info = analyze_password(password);
                cout << "Length: " << info.length << "\n";
                cout << "Pool size: " << info.pool_size << "\n";
                cout << "Entropy (bits): " << info.entropy_bits << "\n";
                cout << "Category: " << info.strength_category << "\n";
                cout << "Time to crack (offline fast): " << human_time(info.time_offline_fast) << "\n";

            } catch (const exception &e) {
                cout << "Error: " << e.what() << "\n";
            }

        } else {
            cout << "Enter a valid choice!\n";
        }
    }

    return 0;
}
