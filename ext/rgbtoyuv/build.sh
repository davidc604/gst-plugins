#!/bin/bash
# 2013 Copyright Remotium Inc.
# Jose Pereira <jose@remotium.com>

#check if someone called the script directly
if [[ ! -L "$0" ]]; then
	echo "Cannot call this file directly. Refer to README on external/scripts/ for more information"
	exit 1
fi

#parse arguments
TARGET=$1
#etc

# load config file from original linked dir
. $(dirname $(readlink $0))/config.sh $*

if [[ $# -ne 1 ]]; then
	echo "Usage: $0 [target]"
	print_supported_plats
	exit 1
fi

check_plat_supported

run_target_build