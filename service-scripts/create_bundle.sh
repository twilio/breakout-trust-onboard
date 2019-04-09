#!/bin/bash

DEFAULT_RASPBIAN_URL=https://downloads.raspberrypi.org/raspbian_lite_latest
DEFAULT_RASPBIAN_IMAGENAME=raspbian_lite_latest

AZURE_REPO=https://github.com/Azure/azure-iot-sdk-c.git
AZURE_BRANCH=hsm_custom_data

RPI_TOOLS_REPO=https://github.com/raspberrypi/tools.git
RPI_TOOLS_BRANCH=master

TOB_REPO=git@github.com:twilio/Breakout_Trust_Onboard_SDK.git
TOB_BRANCH=initial-implementation

WIRELESS_PPP_REPO=https://github.com/twilio/wireless-ppp-scripts.git
WIRELESS_PPP_BRANCH=master

RASPBIAN_DISTRO_URL=http://raspbian.raspberrypi.org/raspbian/
REQUIRED_PACKAGES="libcurl4-openssl-dev libpcap0.8 libssl1.0-dev ppp uuid-dev cmake cmake-data libarchive13 libjsoncpp1 libuv1 liblzo2-2 smstools procmail screen"
REQUIRED_PACKAGES="${REQUIRED_PACKAGES} udhcpd"

find_file () {
	package=$1

	if [ ! -f Packages ]; then
		wget ${RASPBIAN_DISTRO_URL}/dists/stretch/main/binary-armhf/Packages
	fi

	cat Packages | \
	sed -n "/^Package: ${package}\$/,/^Package: .*\$/p" | \
	sed -n "s/Filename: \(.*\)/\1/p"
}

mount_sysroot () {
	mountpoint=$1
	image=$2

	mkdir -p ${mountpoint}
	offset=$(parted -s ${image} unit B p | sed -n 's/^\s*2\s*\([0-9]*\)B.*$/\1/p')

	sudo mount -o loop,offset=${offset},rw ${image} ${mountpoint}
}

# parted,wget?,dpkg,

while getopts 'hm:i:a:b:r:p:' opt; do
	case ${opt} in
		h)
			scriptname=$(basename $0)
			echo "Usage: ${scriptname} [-h] [-m] [-i] [-a] [-b] [-r] [-p]\
			      \
			      Options:\
			        -h Print this help.\
				-m Set mount point. A temporary directory is used by default.\
				-i Raspbian image to use. Raspbian Stretch Lite is downloaded to the current directory by default\
				-a Path to Azure SDK repository. Cloned to the current directory by default.\
				-b Path to Trust Onboard Breakout repository. Cloned to the current directory by default.
				-r Path to Raspberry Pi Tools repository. Cloned to the current directory by default.
				-p Path to Twilio Wireless PPP repository. Cloned to the current directory by default."
			;;
		m)	mount_point=${OPTARG} ;;
		i)	raspbian_image=${OPTARG} ;;
		a)	azure_repo=${OPTARG} ;;
		b)	tob_repo=${OPTARG} ;;
		r)	rpi_tools_repo=${OPTARG} ;;
		p)	wireless_ppp_repo=${OPTARG} ;;
	esac
done

if [ -z "$mount_point" ]; then
	mount_point=$(mktemp -d)
fi

if [ -z "$raspbian_image" ]; then
	echo "Downloading Raspbian image... "
	if [ -f ${DEFAULT_RASPBIAN_IMAGENAME} ]; then
		wget -c ${DEFAULT_RASPBIAN_URL}
	else
		wget ${DEFAULT_RASPBIAN_URL}
	fi

	unpacked_image_name=$(unzip -l ${DEFAULT_RASPBIAN_IMAGENAME} | sed -n 's/^\s*[^ \t]*\s*[^ \t]*\s[^ \t]*\s*\([^ \t]*\.img\).*$/\1/p')
	echo "Extracting Raspbian image... "
	unzip -f ${DEFAULT_RASPBIAN_IMAGENAME}
	raspbian_image=$(pwd)/${unpacked_image_name}
fi


if [ -z "$azure_repo" ]; then
	echo "Cloning Azure IoT SDK... "
	git clone "$AZURE_REPO" --branch ${AZURE_BRANCH} --recursive azure-sdk
	azure_repo=$(pwd)/azure-sdk
fi

if [ -z "$tob_repo" ]; then
	echo "Cloning Trust Onboard SDK... "
	git clone "$TOB_REPO" --branch ${TOB_BRANCH}  --recursive tob-sdk
	tob_repo=$(pwd)/tob-sdk
fi

if [ -z "$rpi_tools_repo" ]; then
	echo "Cloning Raspberry Pi tools... "
	git clone "$RPI_TOOLS_REPO" --branch ${RPI_TOOLS_BRANCH}  --recursive rpi-tools
	rpi_tools_repo=$(pwd)/rpi-tools
fi

if [ -z "$wireless_ppp_repo" ]; then
	echo "Cloning Wireless PPP scripts... "
	git clone "$WIRELESS_PPP_REPO" --branch ${WIRELESS_PPP_BRANCH}  --recursive wireless-ppp
	wireless_ppp_repo=$(pwd)/wireless-ppp
fi

mount_sysroot ${mount_point} ${raspbian_image}

echo "Downloading Debian packages"

mkdir -p debs
pushd debs
for package in ${REQUIRED_PACKAGES}; do
	wget -nc "${RASPBIAN_DISTRO_URL}$(find_file ${package})"
done
popd

echo "Installing Debian packages"

for package in debs/*.deb; do
	sudo dpkg -x ${package} ${mount_point}
done

export PATH=${PATH}:${rpi_tools_repo}/arm-bcm2708/gcc-linaro-arm-linux-gnueabihf-raspbian-x64/bin
export RPI_ROOT=${mount_point}

echo "Building Azure SDK"
pushd ${azure_repo}/build_all/linux
patch -p3 -N << _DONE_
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
./build.sh --toolchain-file ${tob_repo}/device-support/Seeed-LTE_Cat_1_Pi_HAT/toolchain.cmake --provisioning --no_uploadtoblob --install-path-prefix ${RPI_ROOT}/usr
cd ../../cmake/iotsdk_linux
sudo make install
popd

echo "Building Trust Onboard SDK"
pushd ${tob_repo}
mkdir -p cmake
cd cmake
cmake -DCMAKE_TOOLCHAIN_FILE=${tob_repo}/device-support/Seeed-LTE_Cat_1_Pi_HAT/toolchain.cmake -DTOB_DEVICE=Seeed-LTE_Cat_1_Pi_HAT ..
cpack
for package in *.deb; do
	sudo dpkg -x ${package} ${mount_point}
done

popd

sudo ln -s /etc/systemd/system/seeed_lte_hat.service ${RPI_ROOT}/etc/systemd/system/multi-user.target.wants/seeed_lte_hat.service

echo "Installing PPP scripts"
ppp_tempdir=$(mktemp -d)
cp -r ${wireless_ppp_repo}/* ${ppp_tempdir}
pushd ${ppp_tempdir}
sed -i 's/ttyUSB0/ttyACM0/g' peers/twilio
sed -i '/^defaultroute/ a replacedefaultroute' peers/twilio
sed -i 's/\(^OK AT+CGDCONT\)/#&/g' chatscripts/twilio
sudo cp chatscripts/twilio ${mount_point}/etc/chatscripts
sudo cp peers/twilio ${mount_point}/etc/ppp/peers
rm -rf ${ppp_tempdir}
popd

sudo ln -s /etc/systemd/system/twilio_ppp.service ${RPI_ROOT}/etc/systemd/system/multi-user.target.wants/twilio_ppp.service

echo "Setting up smstools"
sudo cp bundle_files/etc/smsd.conf ${RPI_ROOT}/etc/
sudo cp bundle_files/home/pi/sms_received.sh ${RPI_ROOT}/home/pi/
sudo touch ${RPI_ROOT}/home/pi/azure_scope_id.txt
pi_uname=$(cat ${mount_point}/etc/passwd | grep '^pi:' | cut -d ':' -f 3)
dialout_grp=$(cat ${mount_point}/etc/group | grep '^dialout:' | cut -d ':' -f 3)
sudo chown -R ${pi_uname}:${dialout_grp} ${mount_point}/home/pi/azure_scope_id.txt

echo "Setting up udhcpd"
sudo cp bundle_files/etc/udhcpd.conf ${RPI_ROOT}/etc/

echo "Setting up static IP address for conference"
sudo cp bundle_files/etc/network/interfaces ${RPI_ROOT}/etc/network/

# TODO: enable sshd
# TODO: change default pi password
# TODO: change default locale for keyboard to en_US.UTF-8 UTF-8
# TODO: /usr/local/seeed_lte_hat/ directory and py file are missing

echo "Installing sample sources"
sudo rm -rf ${mount_point}/home/pi/azure-sample
sudo cp -r ${tob_repo}/cloud-support/azure-iot ${mount_point}/home/pi/azure-sample
sudo rsync -axvp ${tob_repo} ${mount_point}/home/pi/Breakout_Trust_Onboard_SDK
pi_uname=$(cat ${mount_point}/etc/passwd | grep '^pi:' | cut -d ':' -f 3)
pi_grp=$(cat ${mount_point}/etc/passwd | grep '^pi:' | cut -d ':' -f 4)
sudo chown -R ${pi_uname}:${pi_grp} ${mount_point}/home/pi/{azure-sample,Breakout_Trust_Onboard_SDK}

sudo umount ${mount_point}
rmdir ${mount_point}
