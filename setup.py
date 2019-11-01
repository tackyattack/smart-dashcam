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

if args.aux_unit:
    print('\n\n ***** installing dbus dependencies *****\n\n')
    cmd = 'sudo apt install libdbus-1-dev libdbus-glib-1-dev'
    subprocess.check_call(cmd.split())

print('\n\n ***** installing vlc, ffmpeg, and pip *****\n\n')
cmd = 'sudo apt-get install vlc ffmpeg python-pip'
subprocess.check_call(cmd.split())

print('\n\n ***** installing python libraries *****\n\n')
cmd = 'pip install picamera'
subprocess.check_call(cmd.split())
if args.main_unit:
    cmd = 'pip install PySimpleGUI27 typing'
    subprocess.check_call(cmd.split())
