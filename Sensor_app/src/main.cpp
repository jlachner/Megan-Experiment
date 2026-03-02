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

#include "AtiForceTorqueSensor.h"
#include "exp_robots.h"

using namespace std;

// -----------------------------
// Helpers
// -----------------------------

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

        return value;
    }
}

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
    cout << "Current Pose:\n";
    cout << "  Px[m], Py[m], Pz[m] = " << pose.p.transpose() << "\n";
    cout << "  A(Z)[deg] = " << pose.A_deg
         << ", B(Y)[deg] = " << pose.B_deg
         << ", C(X)[deg] = " << pose.C_deg << "\n\n";
}

void writeMetadata(ofstream& file, int trial, const string& ts, const RobotPoseInfo& pose) {
    file << "# Trial: " << trial << "\n";
    file << "# Timestamp: " << ts << "\n";

    // Joint angles (deg)
    file << "# q[deg]=";
    for (int i = 0; i < pose.q_deg.size(); i++) {
        file << pose.q_deg(i) << (i + 1 < pose.q_deg.size() ? ", " : "");
    }
    file << "\n";

    // Pose (deg)
    file << "# Px[m]=" << pose.p[0] << ", Py[m]=" << pose.p[1] << ", Pz[m]=" << pose.p[2] << "\n";
    file << "# A(Z)[deg]=" << pose.A_deg
         << ", B(Y)[deg]=" << pose.B_deg
         << ", C(X)[deg]=" << pose.C_deg << "\n";
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

// -----------------------------
// main
// -----------------------------
int main() {
    // Robot (FK)
    auto myLBR = make_unique<iiwa14>(1, "Trey", Eigen::Vector3d(0.0, 0.0, 0.071)); // flange->FT offset
    myLBR->init();
    cout << "Robot Initialized.\n\n";

    // FT Sensor
    AtiForceTorqueSensor ftSensor("172.31.1.1");
    cout << "Sensor Activated.\n\n";

    const string outDir = "./prints/";

    // Get pose once (q in deg input, internal FK in rad)
    RobotPoseInfo pose = getRobotPoseFromUser(*myLBR);
    printPoseOnce(pose);

    // Trial loop
    while (true) {
        int trial = getTrialNumber("Enter Trial Number (0 to quit): ");
        if (trial == 0) {
            cout << "Done.\n";
            return 0;
        }

        cout << "Press SPACE to start recording Trial " << trial << "...\n";
        while (getKeyPress() != ' ') {}

        cout << "Recording data for 7 seconds...\n";

        // Use one timestamp for BOTH filename and metadata (so they match)
        string ts = getTimestamp();

        string filename = outDir + "Trial_" + to_string(trial) + "_" + ts + ".txt";

        ofstream File_FT(filename);
        if (!File_FT) {
            cerr << "Error opening file for writing: " << filename << endl;
            continue;
        }

        writeMetadata(File_FT, trial, ts, pose);
        recordTrialFT(File_FT, ftSensor, pose.R, /*duration_sec=*/7.0, /*sample_ms=*/20);

        File_FT.close();
        cout << "Trial " << trial << " completed. Data saved to: " << filename << "\n\n";
    }

    return 0;
}