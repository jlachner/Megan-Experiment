% Easy script to calculate the Forward Kinematics for the iiwa14

%% Cleaning up + Environment Setup
clear; close all; clc;

% Set figure size and attach robot to simulation
robot = iiwa14( 'high' );
robot.init( );
 

% Update kinematics
q = deg2rad( [ 30, 0, 45, 0, 66, 2, 5 ]' );
robot.updateKinematics( q );

% Calculate FK
pos = [ 0, 0, 0.075 ];
H = robot.getForwardKinematics( q, 'bodyID', 7, 'position', pos );
disp( H(1:3,4) );
