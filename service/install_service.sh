#!/bin/bash

if [ ! -d /usr/local/seeed_lte_hat ]; then
    sudo mkdir /usr/local/seeed_lte_hat
fi
sudo cp seeed_lte_hat.py /usr/local/seeed_lte_hat/
sudo cp seeed_lte_hat.service /etc/systemd/system/
sudo systemctl enable seeed_lte_hat
sudo systemctl daemon-reload
sudo systemctl start seeed_lte_hat
