# set variables to be used by all module builders
# expects PACKAGE and VERSION to be set

GROUP='fun3d_users'

GCC_VERSION='6.2.0'
MPT_VERSION='2.23'
INTEL_VERSION='2018.3.222'
PARMETIS_VERSION='4.0.3'
ZOLTAN_VERSION='3.82'
ESP_VERSION='121'

GCC_MODULE="gcc_${GCC_VERSION}"
INTEL_MODULE="intel_${INTEL_VERSION}"
MPT_MODULE="mpt-${MPT_VERSION}"
ESP_MODULE="ESP/${ESP_VERSION}"

PREFIX="/u/shared/${USER}/${GROUP}"     # where everything is anchored

MODULE_ROOT="${PREFIX}/modules"         # where the built artifacts reside
MODFILE_ROOT="${PREFIX}/modulefiles"    # where the modulefiles reside

# artifacts
MODULE_BASE="${MODULE_ROOT}/${PACKAGE}"
MODULE_DEST="${MODULE_BASE}/${VERSION}"

# module system file
MODFILE_BASE="${MODFILE_ROOT}/${PACKAGE}"
MODFILE_DEST="${MODFILE_BASE}/${VERSION}"

. /usr/local/pkgs/modules/init/bash
module use ${MODFILE_ROOT}

skipbuild(){
   if [ -e $2 ] ; then
       echo "$2 exists."
       echo "===Exit $1 build.=="
       exit 
   fi
}
