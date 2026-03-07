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
#include <memory>
#include <stdexcept>
#include <cmath>

#include "AtiForceTorqueSensor.h"
#include "exp_robots.h"

using namespace std;

// -----------------------------
// Function declarations
// -----------------------------
char getKeyPress();
int getTrialNumber(const string& prompt);
int getAngleNumber(const string& prompt);
string getTimestamp();

// -----------------------------
// Helpers
// -----------------------------
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

int getTrialNumber(const string& prompt) {
    while (true) {
        cout << prompt;

        string input;
        cin >> input;

        if (input == "q" || input == "Q") {
            cout << "Done.\n";
            exit(0);
        }

        try {
            int value = stoi(input);

            if (value < 0) {
                cout << "Invalid input. Please enter 0 (quit), a positive trial number, or q to quit.\n";
                continue;
            }

            return value;
        } catch (...) {
            cout << "Invalid input. Please enter a number or q to quit.\n";
        }
    }
}

int getAngleNumber(const string& prompt) {
    while (true) {
        cout << prompt;

        string input;
        cin >> input;

        if (input == "q" || input == "Q") {
            cout << "Done.\n";
            exit(0);
        }

        try {
            int value = stoi(input);
            return value;
        } catch (...) {
            cout << "Invalid input. Please enter a number or q to quit.\n";
        }
    }
}

string getTimestamp() {
    auto now = chrono::system_clock::now();
    time_t t = chrono::system_clock::to_time_t(now);

    tm tm_local;
    localtime_r(&t, &tm_local);

    ostringstream oss;
    oss << put_time(&tm_local, "%Y%m%d_%H%M%S");
    return oss.str();
}

// -----------------------------
// Robot pose bundle
// -----------------------------
struct RobotPoseInfo {
    Eigen::VectorXd q_rad;     // used for FK
    Eigen::VectorXd q_deg;     // stored for metadata / convenience
    Eigen::Matrix4d H;
    Eigen::Vector3d p;
    Eigen::Matrix3d R;
    double A_deg;              // Z
    double B_deg;              // Y
    double C_deg;              // X
};

// Read joint angles [deg], compute FK + pose + Euler ZYX (A=Z, B=Y, C=X) in degrees.
RobotPoseInfo getRobotPoseFromUser(iiwa14& robot) {
    RobotPoseInfo info;
    info.q_rad = Eigen::VectorXd::Zero(robot.nq);
    info.q_deg = Eigen::VectorXd::Zero(robot.nq);
    info.H = Eigen::Matrix4d::Zero();

    const double deg2rad = M_PI / 180.0;
    const double rad2deg = 180.0 / M_PI;

    for (int i = 0; i < robot.nq; i++) {
        cout << "Enter joint angle " << i + 1 << " [deg]: ";
        cin >> info.q_deg(i);
        info.q_rad(i) = info.q_deg(i) * deg2rad;
    }

    info.H = robot.getForwardKinematics(info.q_rad);
    info.p = info.H.block<3, 1>(0, 3);
    info.R = info.H.block<3, 3>(0, 0);

    // Euler ZYX: [yaw(Z), pitch(Y), roll(X)] -> map to A(Z), B(Y), C(X)
    Eigen::Vector3d euler_rad = info.R.eulerAngles(2, 1, 0);
    info.A_deg = euler_rad[0] * rad2deg;
    info.B_deg = euler_rad[1] * rad2deg;
    info.C_deg = euler_rad[2] * rad2deg;

    return info;
}

void printPoseOnce(const RobotPoseInfo& pose) {
    cout << "Current Pose of the tip of the force sensor (NOT robot flange):\n";
    cout << "  Px[m], Py[m], Pz[m] = " << pose.p.transpose() << "\n";
    cout << "  A(Z)[deg] = " << pose.A_deg
         << ", B(Y)[deg] = " << pose.B_deg
         << ", C(X)[deg] = " << pose.C_deg << "\n\n";
}

void writeMetadata(ofstream& file, int trial, const string& ts, const RobotPoseInfo& pose) {
    file << "# Trial: " << trial << "\n";
    file << "# Timestamp: " << ts << "\n";

    file << "# q[deg]=";
    for (int i = 0; i < pose.q_deg.size(); i++) {
        file << pose.q_deg(i) << (i + 1 < pose.q_deg.size() ? ", " : "");
    }
    file << "\n";

    file << "# Px[m]=" << pose.p[0] << ", Py[m]=" << pose.p[1] << ", Pz[m]=" << pose.p[2] << "\n";
    file << "# A(Z)[deg]=" << pose.A_deg
         << ", B(Y)[deg]=" << pose.B_deg
         << ", C(X)[deg]=" << pose.C_deg << "\n";
}

void writeMetadata2(ofstream& file, int trial, const string& ts, int angle) {
    file << "# Trial: " << trial << "\n";
    file << "# Angle[deg]: " << angle << "\n";
    file << "# Timestamp: " << ts << "\n";
    file << "\n";
}

void recordTrialFT(ofstream& file,
                   AtiForceTorqueSensor& ftSensor,
                   const Eigen::Matrix3d& R,
                   double duration_sec = 7.0,
                   int sample_ms = 20) {
    file << "Fx [N], Fy [N], Fz [N], Mx [Nm], My [Nm], Mz [Nm]\n";

    auto start = chrono::steady_clock::now();
    while (chrono::duration<double>(chrono::steady_clock::now() - start).count() < duration_sec) {
        double* data = ftSensor.Acquire();

        Eigen::Map<const Eigen::Matrix<double, 6, 1>> F_ext_ee(data);
        Eigen::Matrix<double, 6, 1> F_ext_0;

        F_ext_0.head<3>() = R * F_ext_ee.head<3>();
        F_ext_0.tail<3>() = R * F_ext_ee.tail<3>();

        for (int i = 0; i < 6; i++) {
            cout << F_ext_0[i] << " ";
            file << F_ext_0[i] << (i < 5 ? ", " : "");
        }
        cout << endl;
        file << "\n";

        this_thread::sleep_for(chrono::milliseconds(sample_ms));
    }
}

void recordTrialFT2(ofstream& file,
                    AtiForceTorqueSensor& ftSensor,
                    double duration_sec = 7.0,
                    int sample_ms = 20) {
    file << "Fx [N], Fy [N], Fz [N], Mx [Nm], My [Nm], Mz [Nm]\n";

    auto start = chrono::steady_clock::now();
    while (chrono::duration<double>(chrono::steady_clock::now() - start).count() < duration_sec) {
        double* data = ftSensor.Acquire();

        for (int i = 0; i < 6; i++) {
            cout << data[i] << " ";
            file << data[i] << (i < 5 ? ", " : "");
        }
        cout << endl;
        file << "\n";

        this_thread::sleep_for(chrono::milliseconds(sample_ms));
    }
}

// -----------------------------
// main
// -----------------------------
int main() {
    // Robot (FK)
    auto myLBR = make_unique<iiwa14>(1, "Trey", Eigen::Vector3d(0.0, 0.0, 0.12)); // flange->FT offset
    myLBR->init();
    cout << "Robot Initialized.\n\n";

    // FT Sensor
    AtiForceTorqueSensor ftSensor("172.31.1.1");
    cout << "Sensor Activated.\n\n";

    const string outDir = "./prints/";

    // Optional pose calculation
    // RobotPoseInfo pose = getRobotPoseFromUser(*myLBR);
    // printPoseOnce(pose);

    while (true) {

        int angle = getAngleNumber("Enter Angle (deg) (or q to quit): ");

        int trial = getTrialNumber("Enter Trial Number (0 to quit, q to quit): ");
        if (trial == 0) {
            cout << "Done.\n";
            return 0;
        }

        cout << "Press SPACE to start recording Trial " << trial << "...\n";
        while (getKeyPress() != ' ') {}

        cout << "Recording data for 7 seconds...\n";

        string ts = getTimestamp();
        string filename = outDir + "Angle_" + to_string(angle) + "_Trial_" + to_string(trial) + "_" + ts + ".txt";

        ofstream File_FT(filename);
        if (!File_FT) {
            cerr << "Error opening file for writing: " << filename << endl;
            continue;
        }

        writeMetadata2(File_FT, trial, ts, angle);
        recordTrialFT2(File_FT, ftSensor, 7.0, 20);

        File_FT.close();
        cout << "Trial " << trial << " completed. Data saved to: " << filename << "\n\n";
    }

    return 0;
}