#!/bin/bash
cat $1 | sed s/}/}\\\n/ | indent -npcs -bl -blf -bls -st - 
