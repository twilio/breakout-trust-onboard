#!/bin/bash

if [ ! -d /usr/local/seeed_lte_hat ]; then
    mkdir /usr/local/seeed_lte_hat
fi
cp service/seeed_lte_hat.py /usr/local/seeed_lte_hat/
cp service/seeed_lte_hat.service /etc/systemd/system/
systemctl enable seeed_lte_hat
systemctl daemon-reload
systemctl start seeed_lte_hat
