#!/bin/bash
SCRIPT_PATH="$(realpath "${BASH_SOURCE[0]}")";
SCRIPT_DIR="$(dirname $SCRIPT_PATH)";
cd $SCRIPT_DIR;
source $SCRIPT_DIR/.venv/bin/activate;
$SCRIPT_DIR/.venv/bin/python -m flask --app index run --port 2855;