#!/bin/bash
rm -r pi_dump/
scp -r pi@raspberrypi.local:~/Documents/lane_detection_exp/ /Users/henrybergin/Documents/COLLEGE/2019/FALL\ SEMESTER/Senior\ Design/smart-dashcam/Lane_Detection/pi_dump
