FROM ubuntu:22.04
FROM jbrhm/WebGPUTutorial 
WORKDIR WebGPUTutorial/
RUN sudo apt install ansible
RUN chmod +x ./ansible/ansible.sh
RUN ./ansible/ansible.sh ./ansible/dev.yml
RUN cmake . -B build
RUN cmake --build build
CMD ["build/Viz"]
