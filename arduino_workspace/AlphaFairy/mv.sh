#!/bin/bash

old_extension="ino"
new_extension="cpp"

# Iterate over files with the old extension in the current directory
for file in *.$old_extension; do
    # Extract the filename without the extension
    filename="${file%.*}"
    # Rename the file with the new extension
    mv "$file" "$filename.$new_extension"
done

echo "File extension change complete."
