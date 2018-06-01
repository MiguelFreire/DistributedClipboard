#!/bin/bash
make clean
make
mkdir ./parent
mkdir ./child
cp clipboard ./parent/clipboard
cp clipboard ./child/clipboard
cp clipboard ./3/clipboard
cp clipboard ./4/clipboard
cp clipboard ./5/clipboard
cp client ./3/client
cp client2 ./5/client2
