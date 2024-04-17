#!/bin/bash

# Get sudo perms
sudo -v

if [ $# -eq 0 ]
  then
    echo "Use $ ./ansible.sh <yml file>"
fi

sudo apt install ansible

ansible-playbook $1
