#!/bin/bash

(time $*) 2>&1 | tee output
