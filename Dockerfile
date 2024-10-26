FROM ubuntu:latest

# Set the user ID and group ID
ARG UID=1069
ARG GID=1000
ARG DOCKER_USER="docker"

RUN apt-get update

RUN apt-get -y install sudo

RUN sudo apt -y install clang-format

WORKDIR /app

RUN mkdir /app/src

ADD src /app/src

COPY ./style.sh .
RUN chmod +x style.sh

CMD /app/style.sh
