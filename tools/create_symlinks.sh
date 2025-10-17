#!/bin/bash
# Создание символических ссылок на common компоненты

echo "Creating symlinks for common components..."

cd ..

# Для каждого node создаем symlink на common
for node in node_ph_ec node_display node_climate node_relay node_water node_template root_node; do
    if [ -d "$node" ]; then
        cd "$node"
        if [ ! -L "common" ]; then
            ln -s ../common common
            echo "✓ Created symlink in $node"
        else
            echo "⊙ Symlink already exists in $node"
        fi
        cd ..
    fi
done

cd tools
echo "Done!"

