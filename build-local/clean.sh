#!/bin/bash
cd "${0%/*}"
set -H
shopt -s extglob
rm -rf !(build.sh|clean.sh|glew-2.1.0)

