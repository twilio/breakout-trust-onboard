#!/bin/bash

# Install qemu on host machine
sudo apt-get install qemu-user-static

DEFAULT_RASPBIAN_URL=https://downloads.raspberrypi.org/raspbian_lite_latest
DEFAULT_RASPBIAN_IMAGENAME=raspbian_lite_latest

AZURE_REPO=https://github.com/Azure/azure-iot-sdk-c.git
AZURE_BRANCH=hsm_custom_data

RPI_TOOLS_REPO=https://github.com/raspberrypi/tools.git
RPI_TOOLS_BRANCH=master

TOB_REPO=git@github.com:twilio/Breakout_Trust_Onboard_SDK.git
TOB_BRANCH=build-bundle

WIRELESS_PPP_REPO=https://github.com/twilio/wireless-ppp-scripts.git
WIRELESS_PPP_BRANCH=master

REQUIRED_PACKAGES="libcurl4-openssl-dev libpcap0.8 libssl1.0-dev ppp uuid-dev cmake cmake-data libarchive13 libjsoncpp1 libuv1 liblzo2-2 smstools procmail screen udhcpd git i2c-tools"

mount_cleanup () {
 	 # Tear-down qemu chroot env
 	 if [ ! -z "${RPI_ROOT}}" ]; then
 		sudo rm ${RPI_ROOT}/usr/bin/qemu-arm-static || true
 		rm ${RPI_ROOT}/tmp/setup.sh || true
 		sudo sed -i 's/^#CHROOT //g' ${RPI_ROOT}/etc/ld.so.preload || true
 		sleep 2
 		sudo umount ${RPI_ROOT}/dev/pts
 		sudo umount ${RPI_ROOT}/dev
 		sudo umount ${RPI_ROOT}/{sys,proc,etc/resolv.conf,boot} || true
 	 fi

	sudo umount ${RPI_BOOT}
	sudo umount ${RPI_ROOT}
	sudo losetup -d ${loop_device}
	rmdir ${RPI_BOOT}
	rmdir ${RPI_ROOT}
}

mount_sysroot () {
	mountpoint_boot=$1
	mountpoint_root=$2
	image=$3

 	 if [ -z "${mountpoint_root}" ]; then
 		fail "Trouble locating mountpoint_root - aborting"
 	 fi

	loop_device=$(sudo losetup --show -f "${image}")
	export loop_device
	sudo partprobe ${loop_device} && \
	sudo mount ${loop_device}p1 ${mountpoint_boot} && \
	sudo mount ${loop_device}p2 ${mountpoint_root}

 	 # Setup chroot qemu env
 	 sudo mount --bind /dev ${mountpoint_root}/dev/ || fail "Unable to mount chroot environment"
 	 sudo mount --bind /sys ${mountpoint_root}/sys/ || fail "Unable to mount chroot environment"
 	 sudo mount --bind /proc ${mountpoint_root}/proc/ || fail "Unable to mount chroot environment"
 	 sudo mount --bind /dev/pts ${mountpoint_root}/dev/pts || fail "Unable to mount chroot environment"
 	 sudo mount --bind /etc/resolv.conf ${mountpoint_root}/etc/resolv.conf || fail "Unable to mount chroot environment"
 	 sudo mount --bind ${mountpoint_boot} ${mountpoint_root}/boot/ || fail "Unable to mount chroot environment"
 	 sudo sed -i 's/^/#CHROOT /g' ${mountpoint_root}/etc/ld.so.preload || fail "Unable to setup chroot environment"
 	 sudo cp /usr/bin/qemu-arm-static ${mountpoint_root}/usr/bin/ || fail "Unable to setup chroot environment"
}

fail () {
	message=$1

	echo "Failed to create image: $message"
	exit 1
}

while getopts 'hn:m:i:a:b:r:p:' opt; do
	case ${opt} in
		h)
			scriptname=$(basename $0)
			echo "Usage: ${scriptname} [-h] [-n] [-m] [-i] [-a] [-b] [-r] [-p]\
			      \
			      Options:\
			        -h Print this help.\
				-n Set boot mount point. A temporary directory is used by default.\
				-m Set root mount point. A temporary directory is used by default.\
				-i Raspbian image to use. Raspbian Stretch Lite is downloaded to the current directory by default\
				-a Path to Azure SDK repository. Cloned to the current directory by default.\
				-b Path to Trust Onboard Breakout repository. Cloned to the current directory by default.
				-r Path to Raspberry Pi Tools repository. Cloned to the current directory by default.
				-p Path to Twilio Wireless PPP repository. Cloned to the current directory by default."
			;;
		n)	mount_point_boot=${OPTARG} ;;
		m)	mount_point_root=${OPTARG} ;;
		i)	raspbian_image=${OPTARG} ;;
		a)	azure_repo=${OPTARG} ;;
		b)	tob_repo=${OPTARG} ;;
		r)	rpi_tools_repo=${OPTARG} ;;
		p)	wireless_ppp_repo=${OPTARG} ;;
	esac
done

if [ -z "$mount_point_boot" ]; then
	mount_point_boot=$(mktemp -d /tmp/boot-XXX)
fi

if [ -z "$mount_point_root" ]; then
	mount_point_root=$(mktemp -d /tmp/root-XXX)
fi

if [ -z "$raspbian_image" ]; then
	echo "Downloading Raspbian image... "
	if [ -f ${DEFAULT_RASPBIAN_IMAGENAME} ]; then
		wget -c ${DEFAULT_RASPBIAN_URL} || true
	else
		wget ${DEFAULT_RASPBIAN_URL} || fail "couldn't download Raspbian"
	fi

	unpacked_image_name=$(unzip -l ${DEFAULT_RASPBIAN_IMAGENAME} | sed -n 's/^\s*[^ \t]*\s*[^ \t]*\s[^ \t]*\s*\([^ \t]*\.img\).*$/\1/p')

	if [ ! -f ${unpacked_image_name} ]; then
		echo "Extracting Raspbian image... "
		unzip -u ${DEFAULT_RASPBIAN_IMAGENAME} || fail "couldn't unzip Raspbian"
	fi
	raspbian_image=$(pwd)/${unpacked_image_name}
fi


if [ -z "$azure_repo" ]; then
  if [ ! -d azure-sdk ]; then
	echo "Cloning Azure IoT SDK... "
  	git clone "$AZURE_REPO" --branch ${AZURE_BRANCH} --recursive azure-sdk
  else
	echo "Using existing Azure IoT SDK."
  fi
  azure_repo=$(pwd)/azure-sdk
fi

if [ -z "$tob_repo" ]; then
  if [ ! -d tob-sdk ]; then
	  echo "Cloning Trust Onboard SDK... "
	  git clone "$TOB_REPO" --branch ${TOB_BRANCH}  --recursive tob-sdk
  else
	  echo "Using existing Trust Onboard SDK... "
  fi
  tob_repo=$(pwd)/tob-sdk
fi

if [ -z "$rpi_tools_repo" ]; then
  if [ ! -d rpi-tools ]; then
	  echo "Cloning Raspberry Pi tools... "
	  git clone "$RPI_TOOLS_REPO" --branch ${RPI_TOOLS_BRANCH}  --recursive rpi-tools
  else
	  echo "Using existing Raspberry Pi tools... "
  fi
  rpi_tools_repo=$(pwd)/rpi-tools
fi

if [ -z "$wireless_ppp_repo" ]; then
  if [ ! -d wireless-ppp ]; then
  	echo "Cloning Wireless PPP scripts... "
  	git clone "$WIRELESS_PPP_REPO" --branch ${WIRELESS_PPP_BRANCH}  --recursive wireless-ppp
  else
  	echo "Using existing Wireless PPP scripts... "
  fi
  wireless_ppp_repo=$(pwd)/wireless-ppp
fi

mount_sysroot ${mount_point_boot} ${mount_point_root} ${raspbian_image} || fail "couldn't mount sysroot"
export RPI_BOOT=${mount_point_boot}
export RPI_ROOT=${mount_point_root}
trap mount_cleanup INT TERM EXIT

if [ -z "${RPI_BOOT}" ]; then
  echo "Error determining RPI_BOOT '${RPI_BOOT}' - aborting."
  exit 1
fi

if [ -z "${RPI_ROOT}" ]; then
  echo "Error determining RPI_ROOT '${RPI_ROOT}' - aborting."
  exit 1
fi

# Generate setup shell script
cat > ${RPI_ROOT}/tmp/setup.sh << _DONE_
echo "Configuring default locale"
# dpkg-reconfigure locales
sed -i 's/^# \(en_US.UTF-8 UTF-8\)/\1/g' etc/locale.gen
sed -i 's/^\(en_GB.UTF-8\)/#\1/g' etc/locale.gen
/usr/sbin/locale-gen

echo "Setting default conference password"
echo -e "build19\nbuild19" | passwd pi

echo "Installing required packages"
apt-get update
apt-get upgrade -y
apt-get install -y ${REQUIRED_PACKAGES}
apt-get autoremove -y
apt-get clean

# Upgrading ssh can lead to host key generation, we don't want this - clean them up if they are created
rm -f etc/ssh/ssh_host_*
_DONE_
sudo chmod +x ${RPI_ROOT}/tmp/setup.sh || fail "Unable to generate setup script"
sudo chroot ${RPI_ROOT} /bin/bash /tmp/setup.sh || fail "Unable to generate setup script"

export PATH=${PATH}:${rpi_tools_repo}/arm-bcm2708/gcc-linaro-arm-linux-gnueabihf-raspbian-x64/bin

echo "Building Azure SDK"
pushd ${azure_repo}/build_all/linux
set +e
patch -p3 -N <<_DONE_ || true
diff --git a/build_all/linux/build.sh b/build_all/linux/build.sh
index 15a65bce0..9d5c60a8a 100755
--- a/build_all/linux/build.sh
+++ b/build_all/linux/build.sh
@@ -127,7 +127,7 @@ process_args \$*
 rm -r -f \$build_folder
 mkdir -p \$build_folder
 pushd \$build_folder
-cmake \$toolchainfile \$cmake_install_prefix -Drun_valgrind:BOOL=\$run_valgrind -DcompileOption_C:STRING="\$extracloptions" -Drun_e2e_tests:BOOL=\$run_e2e_tests -Drun_sfc_tests:BOOL=\$run-sfc-tests -Drun_longhaul_tests=\$run_longhaul_tests -Duse_amqp:BOOL=\$build_amqp -Duse_http:BOOL=\$build_http -Duse_mqtt:BOOL=\$build_mqtt -Ddont_use_uploadtoblob:BOOL=\$no_blob -Drun_unittests:BOOL=\$run_unittests -Dbuild_python:STRING=\$build_python -Dno_logging:BOOL=\$no_logging \$build_root -Duse_prov_client:BOOL=\$prov_auth -Duse_tpm_simulator:BOOL=\$prov_use_tpm_simulator -Duse_edge_modules=\$use_edge_modules
+cmake \$toolchainfile \$cmake_install_prefix -Drun_valgrind:BOOL=\$run_valgrind -DcompileOption_C:STRING="\$extracloptions" -Drun_e2e_tests:BOOL=\$run_e2e_tests -Drun_sfc_tests:BOOL=\$run-sfc-tests -Drun_longhaul_tests=\$run_longhaul_tests -Duse_amqp:BOOL=\$build_amqp -Duse_http:BOOL=\$build_http -Duse_mqtt:BOOL=\$build_mqtt -Ddont_use_uploadtoblob:BOOL=\$no_blob -Drun_unittests:BOOL=\$run_unittests -Dbuild_python:STRING=\$build_python -Dno_logging:BOOL=\$no_logging \$build_root -Duse_prov_client:BOOL=\$prov_auth -Duse_tpm_simulator:BOOL=\$prov_use_tpm_simulator -Duse_edge_modules=\$use_edge_modules -Duse_openssl:bool=ON -Dbuild_provisioning_service_client=ON
 
 if [ "$make" = true ]
 then
_DONE_

set -e
./build.sh --toolchain-file ${tob_repo}/device-support/Seeed-LTE_Cat_1_Pi_HAT/toolchain.cmake --provisioning --no_uploadtoblob --install-path-prefix ${RPI_ROOT}/usr || fail "failed to build Azure SDK"
cd ../../cmake/iotsdk_linux
sudo make install || fail "failed to install Azure SDK"
popd

echo "Building Trust Onboard SDK"
pushd ${tob_repo}
mkdir -p cmake
cd cmake
cmake -DCMAKE_TOOLCHAIN_FILE=${tob_repo}/device-support/Seeed-LTE_Cat_1_Pi_HAT/toolchain.cmake -DTOB_DEVICE=Seeed-LTE_Cat_1_Pi_HAT .. || fail "failed to build Trust Onboard SDK"
cpack || fail "failed to package Trust Onboard SDK"
for package in *.deb; do
	sudo dpkg -x ${package} ${RPI_ROOT} || fail "failed to install Trust Onboard SDK"
done
popd

echo "Installing PPP scripts"
ppp_tempdir=$(mktemp -d)
cp -r ${wireless_ppp_repo}/* ${ppp_tempdir}
pushd ${ppp_tempdir}
sed -i 's/ttyUSB0/ttyACM0/g' peers/twilio
sed -i '/^defaultroute/ a replacedefaultroute' peers/twilio
sed -i 's/\(^OK AT+CGDCONT\)/#&/g' chatscripts/twilio
sudo cp chatscripts/twilio ${RPI_ROOT}/etc/chatscripts
sudo cp peers/twilio ${RPI_ROOT}/etc/ppp/peers
rm -rf ${ppp_tempdir}
popd

echo "Setting up smstools"
sudo cp ${tob_repo}/service-scripts/bundle_files/etc/smsd.conf ${RPI_ROOT}/etc/
sudo cp ${tob_repo}/service-scripts/bundle_files/home/pi/sms_received.sh ${RPI_ROOT}/home/pi/
sudo touch ${RPI_ROOT}/home/pi/azure_id_scope.txt
pi_uname=$(cat ${RPI_ROOT}/etc/passwd | grep '^pi:' | cut -d ':' -f 3)
dialout_grp=$(cat ${RPI_ROOT}/etc/group | grep '^dialout:' | cut -d ':' -f 3)
sudo chown -R ${pi_uname}:${dialout_grp} ${RPI_ROOT}/home/pi/azure_id_scope.txt ${RPI_ROOT}/home/pi/sms_received.sh

echo "Configuring default keyboard layout"
# dpkg-reconfigure keyboard-layout
sudo cp ${tob_repo}/service-scripts/bundle_files/etc/default/keyboard ${RPI_ROOT}/etc/default/

echo "Disabling serial console to avoid conflicts with LTE HAT"
sudo sed -i 's/ console=serial0,115200//g' ${RPI_BOOT}/cmdline.txt

echo "Enabling I2C interface for HAT"
sudo touch ${RPI_ROOT}/etc/modprobe.d/raspi-blacklist.conf
sudo sh -c "echo i2c-dev >> ${RPI_ROOT}/etc/modules"
sudo sed -i 's/^#\(dtparam=i2c_arm=on\)/\1/g' ${RPI_BOOT}/config.txt

echo "Setting up fallback static IP address for conference"
sudo rm -f ${RPI_ROOT}/etc/systemd/system/dhcpcd.service.d/wait.conf
sudo cp ${tob_repo}/service-scripts/bundle_files/etc/dhcpcd.conf ${RPI_ROOT}/etc/

echo "Setting up udhcpd"
sudo sed -i 's/ENABLED="no"/ENABLED="yes"/;s/DHCPD_OPTS="/DHCPD_OPTS="-I 192.168.253.100 /' ${RPI_ROOT}/etc/default/udhcpd
sudo cp ${tob_repo}/service-scripts/bundle_files/etc/udhcpd.conf ${RPI_ROOT}/etc/

echo "Installing initial Breakout_Trust_Onboard_SDK for examples and documentation"
tob_tempdir=$(mktemp -d)
# needs to be done as local user because remote repository is not yet public and ssh identity is needed to clone
git clone "$TOB_REPO" --branch ${TOB_BRANCH} --recursive ${tob_tempdir}
pi_uname=$(cat ${RPI_ROOT}/etc/passwd | grep '^pi:' | cut -d ':' -f 3)
pi_grp=$(cat ${RPI_ROOT}/etc/passwd | grep '^pi:' | cut -d ':' -f 4)
sudo rm -rf ${RPI_ROOT}/home/pi/Breakout_Trust_Onboard_SDK
sudo mv ${tob_tempdir} ${RPI_ROOT}/home/pi/Breakout_Trust_Onboard_SDK
sudo chown -R ${pi_uname}:${pi_grp} ${RPI_ROOT}/home/pi/Breakout_Trust_Onboard_SDK

echo "Enabling services"
cat > ${RPI_ROOT}/tmp/setup.sh << _DONE_
systemctl enable seeed_lte_hat
systemctl enable ssh
systemctl enable udhcpd
systemctl enable twilio_ppp
systemctl disable exim4
_DONE_
sudo chmod +x ${RPI_ROOT}/tmp/setup.sh || fail "Unable to generate setup script"
sudo chroot ${RPI_ROOT} /bin/bash /tmp/setup.sh || fail "Unable to generate setup script"

echo "Successfully created image at ${raspbian_image}"
