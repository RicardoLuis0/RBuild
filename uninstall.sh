#!/usr/bin/env bash
while true; do
    read -p "Uninstall from All users, Local user or Cancel (A/L/C)?" choice
    if [ "$choice" = "a" ] || [ "$choice" = "A" ]; then
        rm /usr/local/bin/RBuild
        break;
    elif [ "$choice" = "l" ] || [ "$choice" = "L" ]; then
        rm ~/bin/RBuild;
        break;
    elif [ "$choice" = "c" ] || [ "$choice" = "C" ]; then
        break;
    else
      echo "invalid choice"
    fi
done
