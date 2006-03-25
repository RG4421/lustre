# oldstyle
MDSNODE=${MDSNODE:-`hostname`}
OSTNODE=${OSTNODE:-`hostname`}
CLIENT=${CLIENT:-client}

FSNAME=lustre
mds_HOST=${mds_HOST:-$MDSNODE}
mdsfailover_HOST=${mdsfailover_HOST}
mgs_HOST=${mgs_HOST:-$MDSNODE}
ost_HOST=${ost_HOST:-$OSTNODE}
ostfailover_HOST=${ostfailover_HOST}
ost2_HOST=${ost2_HOST:-$ost_HOST}
client_HOST=${client_HOST:-$CLIENT}
NETTYPE=${NETTYPE:-tcp}
MGSNID=`h2$NETTYPE $HOSTNAME`

MDSDEV=${MDSDEV:-$ROOT/tmp/${FSNAME}-mdt}
MDSSIZE=${MDSSIZE:-100000}
MDSOPT=${MDSOPT:-"--mountfsoptions=acl"}
OSTDEV=${OSTDEV:-$ROOT/tmp/${FSNAME}-ost0}
OSTSIZE=${OSTSIZE:-200000}
OSTDEV2=${OSTDEV2:-$ROOT/tmp/${FSNAME}-ost1}
FSTYPE=${FSTYPE:-ext3}

MDS_MKFS_OPTS="--mgs --mdt --index=0 --device-size=$MDSSIZE $MDSOPT"
OST_MKFS_OPTS="--ost --index=0 --device-size=$OSTSIZE --mgsnode=`h2$NETTYPE $HOSTNAME` $OSTOPT"
OST2_MKFS_OPTS="--ost --index=1 --device-size=$OSTSIZE --mgsnode=`h2$NETTYPE $HOSTNAME` $OSTOPT"

MDS_MOUNT_OPTS="-o loop"
OST_MOUNT_OPTS="-o loop"
OST2_MOUNT_OPTS="-o loop"

MOUNT=${MOUNT:-/mnt/${FSNAME}}
MOUNT1=${MOUNT1:-$MOUNT}
MOUNT2=${MOUNT2:-${MOUNT}2}
DIR=${DIR:-$MOUNT}
DIR1=${DIR:-$MOUNT1}
DIR2=${DIR2:-$MOUNT2}
MOUNTOPT=${MOUNTOPT:-"user_xattr,acl"}

TIMEOUT=${TIMEOUT:-20}
UPCALL=${UPCALL:-DEFAULT}
PTLDEBUG=${PTLDEBUG:-0x33f0404}
SUBSYSTEM=${SUBSYSTEM:- 0xffb7e3ff}
PDSH=${PDSH:-no_dsh}

STRIPE_BYTES=${STRIPE_BYTES:-1048576}
STRIPES_PER_OBJ=${STRIPES_PER_OBJ:-0}

FAILURE_MODE=${FAILURE_MODE:-SOFT} # or HARD
POWER_DOWN=${POWER_DOWN:-"powerman --off"}
POWER_UP=${POWER_UP:-"powerman --on"}
