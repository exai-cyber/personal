#include <iostream>
#include "password_strength.h"

using namespace std;

int main() {


    cout<<"=== Password Toolkit ===\n";
    cout<<"1) Check password strength\n";
    cout<<"2) Generate strong password\n";
    cout<<"0) Exit\n";
    cout<<"Enter your choice: ";

    int choice;

    while(true) {
        cin>>choice;
        if (choice<0 || choice>2) {
            cout<<"Enter a valid choice!";
        }
        else break;
    }

        if (choice==0) {
            cout<<"Goodbye!";
            exit(0);
        }
        if (choice==1) {
            string password;
            cout<<"Enter your password: ";
            cin>>password;
        }
        //if (choice==2)generateStrongPassword();




    return 0;
}


