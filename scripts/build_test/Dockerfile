FROM ubuntu:latest

# Set the user ID and group ID
ARG UID=1069
ARG GID=1000
ARG DOCKER_USER="docker"

RUN apt-get update

RUN apt-get -y install sudo

RUN sudo apt -y install git ansible wget g++

CMD export CXX=/bin/clang++-18 && export LD_LIBRARY_PATH="" && wget -O bootstrap.sh https://raw.githubusercontent.com/jbrhm/OrientationVisualizer/refs/heads/main/scripts/bootstrap.sh && chmod +x ./bootstrap.sh && ./bootstrap.sh
