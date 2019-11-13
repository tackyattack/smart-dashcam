#!/usr/bin/env python

# Script for setup and building
import argparse
import subprocess
import os

root_path = os.path.dirname(os.path.realpath(__file__))

parser = argparse.ArgumentParser(description='Build and setup script for dashcam units')
parser.add_argument("--aux_unit", action='store_true', help='setup aux unit (zero)')
parser.add_argument("--main_unit", action='store_true', help='setup main unit (Pi 3B+)')
args = parser.parse_args()

def run_cmd_in_path(cmd, path):
    os.chdir(path)
    subprocess.check_call(cmd.split())
    os.chdir(root_path)

if not args.main_unit and not args.aux_unit:
    raise Exception('You need to select type of unit to setup. Use -h for help.')


# both zero and pi3 use the VideoCore dependencies
print('\n\n ***** building VideoCore dependencies *****\n\n')
build_path = '/opt/vc/src/hello_pi'
cmd = 'sudo ./rebuild.sh'
run_cmd_in_path(cmd, build_path)

if args.main_unit:
    print('\n\n ***** building lane detection module *****\n\n')
    # build lane detection OGL library
    build_path = os.path.join(root_path, 'Lane_Detection/src/pilanes/ogl_cv')
    cmd = 'sudo ./rebuild.sh'
    run_cmd_in_path(cmd, build_path)

# both zero and pi3 use the streaming module
print('\n\n ***** building streaming module *****\n\n')
build_path = os.path.join(root_path, 'main_module/dashRecording/Stream')
cmd = 'sudo ./rebuild.sh'
run_cmd_in_path(cmd, build_path)



print('\n\n ***** installing vlc, ffmpeg, and pip *****\n\n')
cmd = 'sudo apt-get install vlc ffmpeg python-pip'
subprocess.check_call(cmd.split())

print('\n\n ***** installing python libraries *****\n\n')
cmd = 'pip install picamera'
subprocess.check_call(cmd.split())
if args.main_unit:
    cmd = 'pip install PySimpleGUI27 typing'
    subprocess.check_call(cmd.split())

# Setup DBUS and Socket libraries on Main and Aux units
print('\n\n ***** Installing DBUS and Socket libraries *****\n\n')
build_path = os.path.join(root_path, 'Interfaces/')
cmd = 'make clean-all setup build'
run_cmd_in_path(cmd, build_path)

# Setup Wi-Fi HOSTING on Main Unit and Setup System Services
print('\n\n ***** Setup Dashcam Wireless Wi-Fi HOSTING and System Services *****\n\n')

if args.main_unit:
    build_path = os.path.join(root_path, 'Interfaces/')
    cmd = 'make pi3_setup'
    run_cmd_in_path(cmd, build_path)

# Setup Wi-Fi Client on Aux Units and Setup System Services
if args.aux_unit:
    build_path = os.path.join(root_path, 'Interfaces/')
    cmd = 'make pi0_setup'
    run_cmd_in_path(cmd, build_path)


print('\n\n ***** Setting up Recording Retrieval *****\n\n')
# Setup Recording Retrieval
if args.main_unit:
    build_path = os.path.join(root_path, 'Recording_Retrieval/FTP')
    cmd = 'sudo ./FTP_Pi3_Bash.sh'
    run_cmd_in_path(cmd, build_path)

if args.aux_unit:
    build_path = os.path.join(root_path, 'Recording_Retrieval/Samba')
    cmd = 'sudo ./sambapiZeroBash.sh'
    run_cmd_in_path(cmd, build_path)

# create recordings folder and give it permissions so script can write to it
# lowercase is local, uppercase is remote
# however, on main unit, recordings go in uppercase since they need to be shared too through FTP
cmd = 'sudo mkdir -p /recordings'
subprocess.check_call(cmd.split())
cmd = 'sudo chmod 777 /recordings'
subprocess.check_call(cmd.split())

cmd = 'sudo mkdir -p /Recordings'
subprocess.check_call(cmd.split())
cmd = 'sudo chmod 777 /Recordings'
subprocess.check_call(cmd.split())

# Setup main services
print('\n\n ***** Setting up main services *****\n\n')
if args.main_unit:
    build_path = os.path.join(root_path, 'setup_files/setup_main_service')
    cmd = 'make pi3_setup'
    run_cmd_in_path(cmd, build_path)

if args.aux_unit:
    build_path = os.path.join(root_path, 'setup_files/setup_aux_service')
    cmd = 'make pi0_setup'
    run_cmd_in_path(cmd, build_path)
