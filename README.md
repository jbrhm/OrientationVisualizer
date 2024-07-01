# Orientation Visualizer
## Overview
The orientation application is meant to be used as a way to visualize some abstract concepts related to orientation.
![alt text](https://github.com/jbrhm/WebGPUTutorial/blob/main/data/orientation.png?raw=true)

## Functionality
- Quaternion Visualization
- SO3 Visualization
- Common Lie Operations
  - Lie Subtract
  - Lie Add (Coming soon)

## Libraries Used
- WebGPU
- Imgui
- Glm
- Eigen

## Installation
- Install Dependencies:
  - Option 1 (Apt))
    - Install ansible `sudo apt install ansible`
    - Clone and cd to the ansible folder
    - Run dependecy install script `chmod +x ansible.sh && ./ansible.sh ./dev.yml`
  - Option 2)
    - Manually install the dependecies that are in `ansible/dev.yml`
- Install Visualizer:
  - Download the debian package [Here](https://github.com/jbrhm/WebGPUTutorial/raw/main/packages/OrientationVisualizer_1.deb)