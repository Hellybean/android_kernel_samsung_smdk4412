#!/sbin/busybox sh
BB="/sbin/busybox"

SEPARATOR() {
	echo "" >> $DEBUG_FILE;
	echo " ---------------------------- " >> $DEBUG_FILE;
	echo "" >> $DEBUG_FILE;
}

dmesg_log() {
	(# dmesg
	echo "dmesg-Info:" > $DEBUG_FILE;
	dmesg >> $DEBUG_FILE;
	SEPARATOR;
	echo "dmesg-Error:" >> $DEBUG_FILE;
	dmesg | grep -i "Error" >> $DEBUG_FILE;
	)&
}

logcat_log() {
	(# logcat
	echo "logcat-Info:" > $DEBUG_FILE_LOGCAT;
	system/bin/logcat -f $DEBUG_FILE_LOGCAT;
	wait 120;
	pkill logcat;
	)&
}


check_mount () {
for i in $($BB seq 1 5) ; do
if $BB test -d /sys/dev/block/179:13 ; then
break
else
echo "Waiting for internal mmc..."
echo $i;
$BB sleep 1
fi
done
}

if $BB grep -q /cache /proc/mounts ; then
	$BB date >/cache/mount.txt
	exec >>/cache/mount.txt 2>&1
else
#/dev/block/mmcblk0p12    /cache            ext4      noatime,nosuid,nodev,journal_async_commit,errors=panic                             wait,check
	$BB mount -t ext4 -o rw /dev/block/mmcblk0p12 /cache
	if ! $BB grep -q /cache /proc/mounts ; then
		echo "mounting /cache with ext4 failed, trying f2fs..."
		$BB mount -t f2fs -o rw /dev/block/mmcblk0p12 /cache
	fi
$BB date >/cache/mount.txt
exec >>/cache/mount.txt 2>&1

echo "/cache wasn't mounted"
fi

DEBUG_FILE=/cache/dmesg.txt
DEBUG_FILE_LOGCAT=/cache/logcat.txt

$BB cp -f /cache/boot.txt /cache/boot.txt.old
$BB cp -f /boot.txt /cache/boot.txt


#check_mount


echo ""
$BB mount

#logcat_log
#dmesg_log

rm -rf $BB
