#!/usr/bin/env bash

# Usage: ./merge_with_headers.sh /path/to/folder output.txt

set -euo pipefail

INPUT_DIR="${1:-.}"
OUTPUT_FILE="${2:-merged_output.txt}"

# Clear/create output file
: > "$OUTPUT_FILE"

# Loop through files in the folder (non-recursive), sorted by name
find "$INPUT_DIR" -maxdepth 1 -type f | sort | while IFS= read -r file; do
  filename="$(basename "$file")"
  {
    echo    # blank line between files
    echo "===== $filename ====="
    echo    # blank line between files
    cat "$file"
    echo    # blank line between files
  } >> "$OUTPUT_FILE"
done

echo "Done. Merged files written to: $OUTPUT_FILE"
