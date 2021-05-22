#!/usr/bin/env bash
while true; do
    read -p "Install for All users, Local user or Cancel (A/L/C)?" choice
    if [ "$choice" = "a" ] || [ "$choice" = "A" ]; then
        cp build/lin/release/bin/RBuild /usr/local/bin/RBuild
        break;
    elif [ "$choice" = "l" ] || [ "$choice" = "L" ]; then
        cp build/lin/release/bin/RBuild ~/bin/RBuild;
        break;
    elif [ "$choice" = "c" ] || [ "$choice" = "C" ]; then
        break;
    else
      echo "invalid choice"
    fi
done
