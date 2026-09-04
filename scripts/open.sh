#!/bin/bash

DUO_IP="192.168.42.1"

if [ "$1" = "-i" ]; then
    echo "Initializing SSH connection to $DUO_IP..."
    ssh-keygen -R "$DUO_IP" 2>/dev/null
    ssh -o StrictHostKeyChecking=no "root@$DUO_IP"
else
    ssh "root@$DUO_IP"
fi
