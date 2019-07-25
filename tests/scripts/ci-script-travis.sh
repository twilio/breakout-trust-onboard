#!/bin/bash

SOURCE_DIR=$(realpath `dirname "${BASH_SOURCE[0]}"`/../..)

mkdir -p cmake
cd cmake
cmake ..
make

# Secret variables are only accessible for PRs from the same repo, so we can't have any repos other than the Twilio one
${SOURCE_DIR}/tests/scripts/hedwig-dispatcher.py --baserepo https://github.com/twilio/Breakout_Trust_Onboard_SDK --prrepo https://github.com/twilio/Breakout_Trust_Onboard_SDK --baseref ${TRAVIS_BRANCH} --prref ${TRAVIS_PULL_REQUEST_BRANCH} ${SECRET_HEDWIG_WORKERS}
