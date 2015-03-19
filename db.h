#ifndef DB_H
#define DB_H

struct dbentry {
	guint64 rfid;
	char *pin;
	char *name;
	int res;
};

#define RES_FAIL 0
#define RES_PASS 1

GHashTable *db_init(void);
int db_read(GHashTable *db, const char *path);
int db_check_rfid(GHashTable *db,guint64 rfid);
int db_check_rfid_pin(GHashTable *db,guint64 rfid,const char *pin);
struct dbentry *db_get_entry(GHashTable *db, guint64 rfid);

#endif
