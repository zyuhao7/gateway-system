#!/bin/bash

echo "=== Verifying Documentation Links in README.md ==="
echo

# 从README提取所有markdown链接
links=$(grep -oP '\[.*?\]\(\K[^)]+' README.md | grep -E '\.md$')

missing=0
found=0

for link in $links; do
    if [ -f "$link" ]; then
        echo "✓ $link"
        found=$((found+1))
    else
        echo "✗ MISSING: $link"
        missing=$((missing+1))
    fi
done

echo
echo "Summary:"
echo "  Found: $found"
echo "  Missing: $missing"

if [ $missing -eq 0 ]; then
    echo
    echo "✅ All documentation links are valid!"
else
    echo
    echo "❌ Some links are broken!"
    exit 1
fi
