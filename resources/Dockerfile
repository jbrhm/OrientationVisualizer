FROM ubuntu:latest

RUN useradd -m docker && echo "docker:docker" | chpasswd && adduser docker sudo

USER docker

RUN apt update
RUN apt -y install sudo
RUN sudo apt -y install git ansible wget
RUN wget -O bootstrap.sh https://raw.githubusercontent.com/jbrhm/OrientationVisualizer/refs/heads/main/scripts/bootstrap.sh && chmod +x ./bootstrap.sh && ./bootstrap.sh
RUN cd OrientationVisualizer

CMD Viz
