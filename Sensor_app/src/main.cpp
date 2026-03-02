#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include <thread>
#include <limits>
#include <termios.h>
#include <unistd.h>
#include <ctime>
#include <iomanip>
#include <sstream>

#include "AtiForceTorqueSensor.h"

using namespace std;

// Get a valid trial number (positive int). Enter 0 to quit.
int getTrialNumber(const string& prompt) {
    int value;
    while (true) {
        cout << prompt;
        cin >> value;

        if (cin.fail()) {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Invalid input. Please enter a number.\n";
            continue;
        }

        if (value < 0) {
            cout << "Invalid input. Please enter 0 (quit) or a positive trial number.\n";
            continue;
        }

        return value; // 0 allowed to quit
    }
}

// Linux single key press (no Enter)
char getKeyPress() {
    struct termios oldt, newt;
    char ch;

    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    ch = getchar();

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    return ch;
}

// Timestamp for filenames: YYYYMMDD_HHMMSS
string getTimestamp() {
    auto now = chrono::system_clock::now();
    time_t t = chrono::system_clock::to_time_t(now);

    tm tm_local;
    localtime_r(&t, &tm_local);

    ostringstream oss;
    oss << put_time(&tm_local, "%Y%m%d_%H%M%S");
    return oss.str();
}


    // ******************************************
    //TODO: Add function to calculate FK and print robot world coordinates of flange
    // ******************************************

int main() {
    // Initialize the force-torque sensor
    // Check the force-torque sensor via HTML interface at http://172.31.1.1
    AtiForceTorqueSensor ftSensor("172.31.1.1");
    cout << "Sensor Activated.\n\n";

    const string outDir = "./prints/";

    while (true) {
        int trial = getTrialNumber("Enter Trial Number (0 to quit): ");
        if (trial == 0) {
            cout << "Done.\n";
            return 0;
        }

        cout << "Press SPACE to start recording Trial " << trial << "...\n";
        while (getKeyPress() != ' ') {}

        cout << "Recording data for 7 seconds...\n";

        // Filename: Trial_<trial>_<YYYYMMDD_HHMMSS>.txt
        string filename = outDir
                        + "Trial_"
                        + to_string(trial)
                        + "_"
                        + getTimestamp()
                        + ".txt";

        ofstream File_FT(filename);
        if (!File_FT) {
            cerr << "Error opening file for writing: " << filename << endl;
            continue;
        }

        // Column headers (unchanged)
        File_FT << "Fx [N], Fy [N], Fz [N], Mx [Nm], My [Nm], Mz [Nm]\n";

        // Data acquisition for 7 seconds (unchanged)
        auto start = chrono::steady_clock::now();
        while (chrono::duration<double>(chrono::steady_clock::now() - start).count() < 7.0) {
            double* data = ftSensor.Acquire();

            // ******************************************
            //TODO: Convert sensor coordinates to robot world coordinates
            // ******************************************

            // Save and print data (unchanged)
            for (int i = 0; i < 6; i++) {
                cout << data[i] << " ";
                File_FT << data[i] << (i < 5 ? ", " : "");
            }
            cout << endl;
            File_FT << endl;

            this_thread::sleep_for(chrono::milliseconds(20)); // ~50Hz sampling (unchanged)
        }

        File_FT.close();
        cout << "Trial " << trial << " completed. Data saved to: " << filename << "\n\n";
    }

    return 0;
}