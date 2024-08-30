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
- Dependencies:
  - Nvidia Drivers: Follow the proper instructions for your operating system.
  - If encountering difficulties do one of the following:
    - Option 1 (Apt for Docker)
      - Install ansible `sudo apt install ansible`
      - Clone/download the contents of the ansible folder and cd to the ansible folder
      - Run dependecy install script `chmod +x ansible.sh && sudo ./ansible.sh ./dev.yml`
    - Option 2
      - Manually install the dependecies that are in `ansible/dev.yml`
- Install Visualizer:
  - Download the debian package [Here](https://github.com/jbrhm/WebGPUTutorial/raw/main/packages/OrientationVisualizer_1.deb)
  - Install using package manager `sudo apt install -f ./OrientationVisualizer_1.deb`
- Source Environment Configuration:
  - `source /opt/OrientationVisualizer/resources/OrientationVizualizer.sh`
- Usage
  - Run `Viz`

## Uninstallation
- Run `sudo apt remove orientationvisualizer`
