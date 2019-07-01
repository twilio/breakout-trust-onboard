This script makes it easy to test using multiple interfaces auto-detected on a single system.

Modify the modems.csv as necessary, for example swapping LARA-R211 and LARA-R203 if using a US module.

# Usage

This tool requires the `pyscard` python3 package.

From the cmake target directory:

    ../tests/test_runner.py ../tests/modems.csv bin/trust_onboard_sdk_tests bin/trust_onboard_ll_tests
