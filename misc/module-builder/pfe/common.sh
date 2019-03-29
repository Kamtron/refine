# set variables to be used by all module builders
# expects PACKAGE and VERSION to be set

GROUP='n1337'

MPT_VERSION='2.17r13'
INTEL_VERSION='2018.3.222'
PARMETIS_VERSION='4.0.3'
ZOLTAN_VERSION='3.82'
ESP_VERSION='113'

INTEL_MODULE="comp-intel/${INTEL_VERSION}"
MPT_MODULE="mpi-hpe/mpt.${MPT_VERSION}"

PREFIX="${HOME}/shared/${GROUP}"        # where everything is anchored

MODULE_ROOT="${PREFIX}/modules"         # where the built artifacts reside
MODFILE_ROOT="${PREFIX}/modulefiles"    # where the modulefiles reside

# artifacts
MODULE_BASE="${MODULE_ROOT}/${PACKAGE}"
MODULE_DEST="${MODULE_BASE}/${VERSION}"

# module system file
MODFILE_BASE="${MODFILE_ROOT}/${PACKAGE}"
MODFILE_DEST="${MODFILE_BASE}/${VERSION}"

. /usr/share/modules/init/bash
module --append use ${MODFILE_ROOT}

