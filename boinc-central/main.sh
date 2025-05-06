#!/bin/bash

DELIMITER="##"

JOINED_ARGS=""
for arg in "$@"; do
  if [ -z "$JOINED_ARGS" ]; then
    JOINED_ARGS="$arg"
  else
    JOINED_ARGS="$JOINED_ARGS $arg"
  fi
done

IFS="$DELIMITER" read -ra COMMANDS <<< "$JOINED_ARGS"
for cmd in "${COMMANDS[@]}"; do
  cmd=$(echo "$cmd" | sed -e 's/^[[:space:]]*//' -e 's/[[:space:]]*$//')

  if [ -n "$cmd" ]; then
    echo "Running command: $cmd" | tee -a output
    echo "----------------------------------------" | tee -a output
    (time $cmd) 2>&1 | tee -a output
    echo "----------------------------------------" | tee -a output
    echo "" | tee -a output
  fi
done
