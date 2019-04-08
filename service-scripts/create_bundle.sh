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
REQUIRED_PACKAGES="libcurl4-openssl-dev libpcap0.8 libssl1.0-dev ppp uuid-dev cmake cmake-data libarchive13 libjsoncpp1 libuv1 liblzo2-2"

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
		r)	wireless_ppp_repo=${OPTARG} ;;
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
	wget "${RASPBIAN_DISTRO_URL}$(find_file ${package})"
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

echo "Installing sample sources"
sudo rm -rf ${mount_point}/home/pi/azure-sample
sudo cp -r ${tob_repo}/cloud-support/azure-iot ${mount_point}/home/pi/azure-sample
pi_uname=$(cat ${mount_point}/etc/passwd | grep '^pi:' | cut -d ':' -f 3)
pi_grp=$(cat ${mount_point}/etc/passwd | grep '^pi:' | cut -d ':' -f 4)
sudo chown -R ${pi_uname}:${pi_grp} ${mount_point}/home/pi/azure-sample

sudo umount ${mount_point}
rmdir ${mount_point}
