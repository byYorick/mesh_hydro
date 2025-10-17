#!/bin/bash
# Backup and Restore PostgreSQL database

DB_NAME="hydro_system"
DB_USER="postgres"
BACKUP_DIR="./backups"

# Создать директорию для backup если нет
mkdir -p "$BACKUP_DIR"

case "$1" in
    backup)
        TIMESTAMP=$(date +%Y%m%d_%H%M%S)
        BACKUP_FILE="$BACKUP_DIR/hydro_backup_$TIMESTAMP.sql"
        
        echo "Creating backup..."
        pg_dump -U "$DB_USER" "$DB_NAME" > "$BACKUP_FILE"
        
        if [ $? -eq 0 ]; then
            echo "✓ Backup created: $BACKUP_FILE"
            
            # Gzip для экономии места
            gzip "$BACKUP_FILE"
            echo "✓ Compressed: ${BACKUP_FILE}.gz"
        else
            echo "✗ Backup failed"
            exit 1
        fi
        ;;
        
    restore)
        if [ -z "$2" ]; then
            echo "Usage: $0 restore <backup_file>"
            exit 1
        fi
        
        BACKUP_FILE="$2"
        
        if [ ! -f "$BACKUP_FILE" ]; then
            echo "✗ Backup file not found: $BACKUP_FILE"
            exit 1
        fi
        
        echo "Restoring from backup..."
        echo "WARNING: This will drop the existing database!"
        read -p "Continue? (yes/no): " confirm
        
        if [ "$confirm" != "yes" ]; then
            echo "Restore cancelled"
            exit 0
        fi
        
        # Если файл .gz, распаковать
        if [[ "$BACKUP_FILE" == *.gz ]]; then
            gunzip -c "$BACKUP_FILE" | psql -U "$DB_USER" "$DB_NAME"
        else
            psql -U "$DB_USER" "$DB_NAME" < "$BACKUP_FILE"
        fi
        
        if [ $? -eq 0 ]; then
            echo "✓ Database restored successfully"
        else
            echo "✗ Restore failed"
            exit 1
        fi
        ;;
        
    list)
        echo "Available backups:"
        ls -lh "$BACKUP_DIR"
        ;;
        
    *)
        echo "Usage: $0 {backup|restore|list}"
        echo ""
        echo "Commands:"
        echo "  backup              Create new backup"
        echo "  restore <file>      Restore from backup file"
        echo "  list                List available backups"
        exit 1
        ;;
esac

