#!/bin/bash

# Script to fix all formatting issues in the asus-armoury.c file

echo "Fixing code formatting issues..."

# Use sed to fix all the formatting issues at once
sed -i '
    # Remove trailing whitespace
    s/[ \t]*$//g
    
    # Fix indentation: replace leading spaces with tabs
    s/^    /\t/g
    s/^\t    /\t\t/g
    s/^\t\t    /\t\t\t/g
    s/^\t\t\t    /\t\t\t\t/g
    s/^\t\t\t\t    /\t\t\t\t\t/g
    
    # Fix additional indentation levels
    s/^        /\t\t/g
    s/^            /\t\t\t/g
    s/^                /\t\t\t\t/g
    s/^                    /\t\t\t\t\t/g
    
' asus-armoury.c

echo "Formatting fixes applied!"

