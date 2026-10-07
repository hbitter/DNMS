#! /bin/bash

### Install packets
echo ">>>>> installation of dnms as a systemd service starts	             <<<<<"
read -r -p "Run apt-get update/upgrade and install required packages? [y/N] " answer
if [[ "$answer" =~ ^[Yy]$ ]]; then
    echo ">>>>> update of system and installation of necessary packets  <<<<<"
    sudo apt-get update -y
    sudo apt-get upgrade -y
    sudo apt-get install libssl-dev libconfig-dev pigpio libmosquitto-dev libiw-dev -y
    sudo apt-get autoremove
else
    echo ">>>>> skipping apt-get update/upgrade                                         <<<<<"
fi

### compile dnms
echo ">>>>> compilation of dnms                                                         <<<<<"
current_dir="$(pwd)"
suchmuster="uhuuhu"
chmod +x compile
./compile
chmod +x dnms

### install dnms.service as systemd service
echo ">>>>> install dnms.service as systemd service                          <<<<<"
sudo systemctl stop dnms.service
sudo systemctl disable dnms.service
sed  "s!$suchmuster!$current_dir!g" dnms.service > dnms.service2
sudo cp dnms.service2 /lib/systemd/system/dnms.service
sudo chmod 644 /lib/systemd/system/dnms.service
rm dnms.service2
sudo systemctl daemon-reload
sudo systemctl enable dnms.service
sudo systemctl restart dnms.service
echo ">>>>> dnms.service should be installed and running now	        <<<<<"
echo ">>>>>	                                                                                        <<<<<"
echo ">>>>> there is a 10 seconds wait for everything to be ready    <<<<<"
echo ">>>>>	                                                                                        <<<<<"
sleep 10
echo ">>>>> to check the status of dnms.service, input:                    <<<<<"
echo ">>>>> sudo systemctl status dnms.service                               <<<<<"
echo ">>>>> ------------------------------------------------------                          <<<<<"
