FROM ubuntu:latest

RUN apt update
RUN apt -y install sudo
RUN sudo apt -y install git ansible wget
RUN git clone https://github.com/jbrhm/OrientationVisualizer.git
RUN cd OrientationVisualizer

RUN useradd -m docker && echo "docker:docker" | chpasswd && adduser docker sudo

USER docker
RUN ./build.sh
