#!/bin/bash

# Set user and expiry
USER_NAME="akash"
EXPIRY_TIME=$(date -u -d "+2 minutes" +"%Y-%m-%dT%H:%M:%SZ")
#EXPIRY_TIME=$(date -u +"%Y-%m-%dT%H:%M:%SZ")

CURRENT_TIME=$(date -u +"%Y-%m-%dT%H:%M:%SZ")


# Write license file
echo "LICENSE_USER=$USER_NAME" > license.txt
echo "LICENSE_EXPIRES=$EXPIRY_TIME" >> license.txt

echo $CURRENT_TIME
