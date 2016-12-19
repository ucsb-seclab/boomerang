#!/bin/sh

# clean up old copies

if [ -e hisi-idt.elf ]; then
    rm hisi-idt.elf
fi

if [ -e hisi-idt.exe ]; then
    rm hisi-idt.exe
fi

if [ -e hisi-idt.pyc ]; then
    rm hisi-idt.pyc
fi

if [ -d out ]; then
    rm out/*
else
    mkdir out
fi

# generate new executable file

echo "begin to generate elf file..."

nuitka --recurse-all --output-dir=./out hisi-idt.py
rm -rf ./out/hisi-idt.build

mv ./out/hisi-idt.exe ./out/hisi-idt.elf

echo "Done"

echo "begin to generate pyc file..."

python -m py_compile hisi-idt.py
mv hisi-idt.pyc ./out

echo "Done"
