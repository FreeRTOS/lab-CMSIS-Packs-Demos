#! /bin/bash
d=$(dirname $0)
c="cpackget -q pack add -a"
# Use the single source of truth to get packages
plist=$(grep pack: $d/IoT.csolution.yml  | sed -e 's/\s//g' -e s'/-pack://g')

for pkg in $plist; do
	$c $pkg
done
