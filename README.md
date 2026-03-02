# Megan Experiment – Force/Torque Recording Application

This application records force and torque data from an ATI Force–Torque sensor
and transforms them into the robot world frame using forward kinematics of a
KUKA iiwa14 model.

The robot pose (position and orientation) is computed once per session and
stored as metadata. Force–torque values are recorded continuously during each trial.

------------------------------------------------------------
PROJECT STRUCTURE
------------------------------------------------------------

Megan-Experiment/
└── Sensor_app/
    ├── src/
    │   ├── CMakeLists.txt
    │   └── main.cpp
    ├── build/          (created by CMake)
    └── prints/         (output files)

------------------------------------------------------------
REQUIREMENTS (Linux)
------------------------------------------------------------

- CMake
- GCC / G++
- Eigen
- ATI Force-Torque sensor accessible at 172.31.1.1

Check installation:

    cmake --version
    g++ --version

If missing:

    sudo apt update
    sudo apt install cmake build-essential

------------------------------------------------------------
BUILD INSTRUCTIONS
------------------------------------------------------------

1) Navigate to the application directory:

    cd ~/Desktop/Megan-Experiment/Sensor_app

2) Configure the project (only required once or after modifying CMakeLists.txt):

    cmake -S src -B build

3) Compile the program:

    cmake --build build

------------------------------------------------------------
RUN THE APPLICATION
------------------------------------------------------------

    ./build/2671_app

------------------------------------------------------------
PROGRAM WORKFLOW
------------------------------------------------------------

1) Robot model initializes (iiwa14 with flange-to-FT offset).
2) User enters joint angles in degrees.
3) The program:
   - Converts joint angles internally to radians.
   - Computes forward kinematics.
   - Computes flange position and Euler angles.
4) Pose is printed once to the terminal.
5) For each trial:
   - User presses SPACE to start recording.
   - Force–torque data are recorded for 7 seconds (~50 Hz).
   - Forces and torques are rotated into the robot world frame.
   - Data are saved to file.

------------------------------------------------------------
OUTPUT FILE FORMAT
------------------------------------------------------------

Each trial generates:

    prints/Trial_<trialNumber>_<YYYYMMDD_HHMMSS>.txt

The file contains:

Metadata (written once at top of file):
- Trial number
- Timestamp
- Joint angles [deg]
- Position Px, Py, Pz [m]
- Orientation A(Z), B(Y), C(X) [deg]

Data columns (continuous samples):

    Fx [N], Fy [N], Fz [N], Mx [Nm], My [Nm], Mz [Nm]

Only force–torque values are written continuously.
Position and orientation are stored once as metadata.

------------------------------------------------------------
AFTER EDITING THE CODE
------------------------------------------------------------

If you modify only .cpp or .h files:

    cmake --build build
    ./build/2671_app

You do NOT need to re-run CMake configuration unless:
- You changed CMakeLists.txt
- You added or removed source files
- You deleted the build folder

------------------------------------------------------------
OUTPUT DIRECTORY
------------------------------------------------------------

Recorded data are stored in:

    prints/

If the folder does not exist, create it once:

    mkdir -p prints

------------------------------------------------------------
TROUBLESHOOTING
------------------------------------------------------------

If you see:

    Error opening file for writing ...

Ensure:

1) The prints directory exists
2) You have write permissions

Check permissions:

    ls -ld prints
    touch prints/test.txt
    rm prints/test.txt

------------------------------------------------------------
NOTES
------------------------------------------------------------

- Joint angles are entered in degrees but internally converted to radians.
- Euler angles follow ZYX convention:
    A = rotation about Z
    B = rotation about Y
    C = rotation about X
- A single timestamp is used per trial to ensure filename and metadata match.
- Forces are transformed into the robot world frame using the FK rotation matrix.