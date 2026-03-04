#!/bin/bash
#
# File name: bld.sh
# Author: Neil Johnson
# Date created: 2026-03-01 06:51:26
# Date modified: 2026-03-02 07:45:55
# ------

trap_error() {
  echo "Error: ${BASH_SOURCE[1]} around line ${BASH_LINENO[0]}"
}

set -e
set -o errtrace
trap trap_error ERR

SCRIPT_PATH=`realpath $(dirname $0)`



BLD_ROOT_DIR="${SCRIPT_PATH}/buildroot"

OUTPUT_DIR="${SCRIPT_PATH}/output"

BR_EXTERNAL_DIR="${SCRIPT_PATH}/br-external"

EXTERNAL_DESC_PATH="${BR_EXTERNAL_DIR}/external.desc"

do_init() {
  git_dir="${SCRIPT_PATH}/.git"

  echo "Going to remove following: ${git_dir}"
  echo -n "Proceed? [y/n]: "
  read -r -n 1 ans
  echo -e "\n"
  if [[ ${ans} == "y" ]]; then
    rm -rf ${git_dir}
  fi
  git init --quiet .
}

do_setup() {
  BLD_ROOT_VERSION="2025.11.1"
  BLD_ROOT_TAR_BALL="${BLD_ROOT_VERSION}.tar.gz"
  BLD_ROOT_SRC_DIR="buildroot-${BLD_ROOT_VERSION}"

  name="${1^^}"
  if [[ -z ${name} ]]; then
    echo "Missing name"
    exit 1
  fi

  desc="${2:-Details to follow...}"

  echo -e "name: ${name}\ndesc: ${desc}" > ${EXTERNAL_DESC_PATH}

  #check if dir exists
  if [[ ! -d ${BLD_ROOT_SRC_DIR} ]]; then
    if [[ ! -e ${BLD_ROOT_TAR_BALL} ]]; then
      echo "${BLD_ROOT_TAR_BALL} is missing, attempting to download"
      wget https://github.com/buildroot/buildroot/archive/refs/tags/${BLD_ROOT_TAR_BALL}
      if [[ ! -e ${BLD_ROOT_TAR_BALL} ]]; then
        echo "Download failed"
        exit 1
      fi
    fi
    echo "Extracting ${BLD_ROOT_TAR_BALL}"
    tar -xf ${BLD_ROOT_TAR_BALL}
    rm -vf ${BLD_ROOT_TAR_BALL}
  fi

  #check it's symlinked
  if [[ ! -h ${BLD_ROOT_DIR} ]]; then
    ln -s ${BLD_ROOT_SRC_DIR} ${BLD_ROOT_DIR}
  fi

}

do_make_manual() {
  cd ${BLD_ROOT_DIR}
  make O=${OUTPUT_DIR} manual
}

do_make_list_defconfigs() {
  cd ${BLD_ROOT_DIR}
  make BR2_EXTERNAL=${BR_EXTERNAL_DIR} list-defconfigs
}

# $1 - which menuconfig to run
do_make_menuconfig() {
  cd ${BLD_ROOT_DIR}
  make O=${OUTPUT_DIR} BR2_EXTERNAL=${BR_EXTERNAL_DIR} "$1"
}

do_make_defconfig() {
  def_cfg_name="${1}"
  if [[ -z ${def_cfg_name} ]]; then
    echo "Need to specify a default config"
    exit 1
  fi
  def_cfg_file="${BR_EXTERNAL_DIR}/configs/${def_cfg_name}"
  if [[ ! -e  ${def_cfg_file} ]]; then
    echo "No config file for ${def_cfg_name}"
    exit 1
  fi

  cd ${BLD_ROOT_DIR}
  make O=${OUTPUT_DIR} BR2_EXTERNAL=${BR_EXTERNAL_DIR} BR2_DEFCONFIG=${def_cfg_file} defconfig
}

do_make_build() {
  cd ${BLD_ROOT_DIR}
  make V=1 O=${OUTPUT_DIR} BR2_EXTERNAL=${BR_EXTERNAL_DIR} all
}

do_make_clean() {
  cd ${BLD_ROOT_DIR}
  make V=1 O=${OUTPUT_DIR} BR2_EXTERNAL=${BR_EXTERNAL_DIR} clean
}

do_make_savedefconfig() {
  cd ${BLD_ROOT_DIR}
  make V=1 O=${OUTPUT_DIR} BR2_EXTERNAL=${BR_EXTERNAL_DIR} savedefconfig
}

do_make_package() {
  package="${1}"
  if [[ -z ${package} ]]; then
    echo "Need to specify package"
    exit 1
  fi

  cd ${BLD_ROOT_DIR}
  make V=1 O=${OUTPUT_DIR} BR2_EXTERNAL=${BR_EXTERNAL_DIR} "${package}"
}

# Start processing here
#----------------------

case "$1" in

  init)
  do_init
  ;;

  setup)
  do_setup "$2" "$3" "$4"
  ;;

  build)
  do_make_build
  ;;

  clean)
  do_make_clean
  ;;

  defconfig)
  do_make_defconfig "$2"
  ;;

  list-defconfigs)
  do_make_list_defconfigs
  ;;

  manual)
  do_make_manual
  ;;

  menuconfig | linux-menuconfig | busybox-menuconfig)
  do_make_menuconfig "$1"
  ;;

  package)
  do_make_package "$2"
  ;;

  savedefconfig)
  do_make_savedefconfig
  ;;

  *)
    echo "Usage: $0 {build, busybox-menuconfig, clean, defconfig, init, linux-menuconfig, list-defconfigs, manual, menuconfig, package, savedefconfig, setup}"
    exit 1
    ;;

esac
