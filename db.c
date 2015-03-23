#include <stdio.h>

#include <glib.h>

#include "db.h"

void db_entry_free(gpointer ptr) {
	struct dbentry *entry = ptr;
	g_free(entry->full_name);
	g_free(entry->nickname);
	g_free(entry->pin);
	g_free(entry);
}


/**
 * Read database from disk.
 * \param db Hashtable to load the data to.
 * \param path Pathname to load from.
 * \return 0 on success, -1 on failure. */
int db_read(GHashTable *db, const char *path) {
	if (db == NULL) return -1;

	GKeyFile *keyfile = g_key_file_new();
	if (!g_key_file_load_from_file(keyfile,path,G_KEY_FILE_NONE,NULL)) {
		// Failed to read the keyfile.
		printf("Error: Failed to read the keyfile: %s\n",path);
		g_key_file_free(keyfile);
		return -1;
	}

	gchar **ids = g_key_file_get_keys(keyfile,"RFID",NULL,NULL);
	if (ids == NULL) {
		// No entries, not strictly an error..
		printf("Warning: Keyfile is empty.\n");
		g_key_file_free(keyfile);
		return 0;
	}

	int i = 0;
	gchar *id;
	while ((id = ids[i++]) != NULL) {
		gchar *paramstr = g_key_file_get_string(keyfile,"RFID",id,NULL);
		if (paramstr == NULL) continue;
		gchar **fields = g_strsplit(paramstr,",",4);
		if (fields == NULL) {
			g_free(paramstr);
			continue;
		}
		if (fields[0] == NULL) {
			g_free(paramstr);
			g_strfreev(fields);
			continue;
		}
		struct dbentry *entry = g_malloc(sizeof(struct dbentry));
		entry->rfid = g_ascii_strtoull(id,NULL,10);
		entry->res = g_ascii_strtoull(fields[0],NULL,10);
		if (entry->res != RES_PASS) entry->res = RES_FAIL;
		entry->full_name = NULL;
		entry->nickname = NULL;
		entry->pin = NULL;

		if (fields[1] != NULL) {
			entry->pin = g_strdup(fields[1]);
		}
		if (fields[2] != NULL) {
			entry->full_name = g_strdup(fields[2]);
		}
		if (fields[3] != NULL) {
			entry->nickname = g_strdup(fields[3]);
		}
		if (entry->rfid == 0) {
			db_entry_free(entry);
			g_strfreev(fields);
			g_free(paramstr);
			continue;
		}

		// Insert entry into the hashtable.
		g_hash_table_replace(db,&entry->rfid,entry);
	}
	g_strfreev(ids);

	printf("Keyfile read ok\n");
	g_key_file_free(keyfile);
	return 0;
}

/**
 * Initialize a new database hashtable.
 * \return Newly allocated hashtable. */
GHashTable *db_init(void) {
	return g_hash_table_new_full(g_int64_hash,g_int64_equal,NULL,db_entry_free);
}

/**
 * Check whether a RFID passes.
 * \param db Database to check from.
 * \param rfid RFID to check.
 * \return RES_FAIL if not allowed, RES_PASS if allowed. */
int db_check_rfid(GHashTable *db,guint64 rfid) {
	if ((db == NULL) || (rfid == 0)) return RES_FAIL; // Invalid request.
	struct dbentry *entry = g_hash_table_lookup(db,&rfid);
	if (entry == NULL) return RES_FAIL; // No such entry.
	if ((entry->pin != NULL) && (entry->pin[0] != '\0')) {
		// Pin specified but not given.
		return RES_FAIL;
	}
	// Return stored result.
	return entry->res;
}

/**
 * Check whether a RFID+PIN passes.
 * \param db Database to check from.
 * \param rfid RFID to check.
 * \param pin PIn number to check.
 * \return RES_FAIL if not allowed, RES_PASS if allowed. */
int db_check_rfid_pin(GHashTable *db,guint64 rfid,const char *pin) {
	if ((db == NULL) || (rfid == 0)) return RES_FAIL; // Invalid request.
	struct dbentry *entry = g_hash_table_lookup(db,&rfid);
	if (entry == NULL) return RES_FAIL; // No such entry.
	if (entry->pin == NULL)  {
		if (pin == NULL) return entry->res; // No pin given or exists.
		// Pin given but doesn't exist.
		return RES_FAIL;
	}
	if (g_ascii_strcasecmp(entry->pin,pin) != 0) {
		return RES_FAIL; // Entered PIN is wrong.
	}
	// Return stored result.
	return entry->res;
}

/**
 * Get an entry from the RFID database by account ID
 * \param db Database to fetch from
 * \param rfid ID of account
 * \return Pointer to DB entry or NULL if not found. */
struct dbentry *db_get_entry(GHashTable *db, guint64 rfid) {
	return g_hash_table_lookup(db, &rfid);
}

