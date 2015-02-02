#!/bin/bash

VKMM_PATH=$(dirname $0)

export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$VKMM_PATH/lib

$VKMM_PATH/bin/vkmm