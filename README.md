==========================================================
Megan Experiment – Force/Torque Recording Application
==========================================================

This application records force and torque data from the ATI
Force-Torque sensor for experimental trials.

----------------------------------------------------------
PROJECT STRUCTURE
----------------------------------------------------------

Megan-Experiment/
└── Sensor_app/
    ├── src/
    │   ├── CMakeLists.txt
    │   └── main.cpp
    ├── build/          (created by CMake)
    └── prints/         (output files)

----------------------------------------------------------
REQUIREMENTS (Linux)
----------------------------------------------------------

- CMake
- GCC / G++

Check installation:

    cmake --version
    g++ --version

If missing:

    sudo apt update
    sudo apt install cmake build-essential

----------------------------------------------------------
BUILD INSTRUCTIONS
----------------------------------------------------------

1) Navigate to the application directory:

    cd ~/Desktop/Megan-Experiment/Sensor_app

2) Configure the project (only required once,
   or after modifying CMakeLists.txt):

    cmake -S src -B build

3) Compile the program:

    cmake --build build

----------------------------------------------------------
RUN THE APPLICATION
----------------------------------------------------------

    ./build/2671_app

You should see:

    Sensor Activated.
    Enter Trial Number (0 to quit):

----------------------------------------------------------
AFTER EDITING THE CODE
----------------------------------------------------------

If you modify only .cpp or .h files:

    cmake --build build
    ./build/2671_app

You do NOT need to re-run CMake configuration unless:
- You changed CMakeLists.txt
- You added or removed source files
- You deleted the build folder

----------------------------------------------------------
OUTPUT FILES
----------------------------------------------------------

Recorded data are stored in:

    prints/

If the folder does not exist, create it once:

    mkdir -p prints

----------------------------------------------------------
TROUBLESHOOTING
----------------------------------------------------------

If you see:

    Error opening file for writing ...

Ensure:

1) The prints directory exists
2) You have write permissions

Check permissions:

    ls -ld prints
    touch prints/test.txt
    rm prints/test.txt

----------------------------------------------------------
END
----------------------------------------------------------
 
