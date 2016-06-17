#!/bin/sh

tmp=tmp.file
iconv -f gbk -t utf-8 $1 > $tmp
mv $tmp $1

