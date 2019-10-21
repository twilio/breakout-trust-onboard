#!/bin/bash

set -e

if [ $EUID -ne 0 ]; then
	echo "This tool must be run as root"
	exit 1
fi

if [[ ${TRAVIS_TAG} == v* ]]; then
	VERSION=${TRAVIS_TAG:1}
else
	echo "Not in a tag, exit"
	exit 1
fi

dist=${1}

mkdir -p debs_to_deploy_${dist}

if [ ! -d raspbian-deb-builder ]; then
	git clone https://github.com/oytis/raspbian-deb-builder
fi

pushd ./raspbian-deb-builder

DEBS=$(./cross-build.sh ${dist} breakout-tob-sdk:${TRAVIS_TAG}:${VERSION})

popd

for d in "${DEBS}"; do
	mv ${d} debs_to_deploy_${dist}
done


NUM_DEBS=$(ls -1q debs_to_deploy_${dist}/* | wc -l)
if [ ${NUM_DEBS} -ne 1 ]; then
	echo "Exactly one debian file for ${dist} expected"
	ls -1q debs_to_deploy_${dist}/*
	exit 1
fi

cat >deploy-${dist}.json <<EOF
{
  "package": {
    "name":"trust-onboard-sdk",
    "repo":"wireless",
    "subject":"twilio",
    "licenses": ["Apache-2.0"],
    "public_stats": true,
    "public_download_numbers": false
  },

  "version": {
    "name":"${VERSION}",
    "gpgSign":true
  },

  "files":
    [{"includePattern": "debs_to_deploy_${dist}/(.*\.deb)", "uploadPattern": "pool/main/m/trust-onboard-sdk/\$1",
      "matrixParams": {
        "deb_distribution": "${dist}",
        "deb_component": "main",
        "deb_architecture": "armhf"}
    }],
  "publish": true
}
EOF
