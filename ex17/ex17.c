#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

struct Address {
	char *email;
	char *name;
	int id;
	int set;
};

struct Database {
	struct Address **rows;
	int max_data;
	int max_rows;
};

struct Connection {
	FILE *file;
	struct Database *db;
};

void Database_close(struct Connection *conn);

void die(struct Connection *conn, const char *message)
{
	if (errno) {
		perror(message);
	} else {
		printf("ERROR: %s\n", message);
	}

	if (conn)
		Database_close(conn);

	exit(1);
}

void Address_print(struct Address *addr)
{
	printf("%d %s %s\n", addr->id, addr->name, addr->email);
}

void Database_load(struct Connection *conn, int data, int rows)
{
	int rc = 0;
	int i = 0;

	if (data < 1) {
		rc = fread(&conn->db->max_data, sizeof(int), 1, conn->file);
		if (rc != 1)
			die(conn, "Failed to load database.");
	} else {
		conn->db->max_data = data;
	}

	if (rows < 1) {
		rc = fread(&conn->db->max_rows, sizeof(int), 1, conn->file);
		if (rc != 1)
			die(conn, "Failed to load database.");
	} else {
		conn->db->max_rows = rows;
	}

	conn->db->rows = malloc(sizeof(struct Address *)*conn->db->max_rows);
	if (!conn->db->rows)
		die(conn, "Memory error");

	for (i=0; i < conn->db->max_rows; i++) {

		struct Address *row = malloc(sizeof(struct Address));

		row->email = malloc(sizeof(char)*conn->db->max_data);
		row->name = malloc(sizeof(char)*conn->db->max_data);

		rc = fread(row->email, sizeof(char)*conn->db->max_data, 1, conn->file);
		if (rc != 1)
			die(conn, "Failed to load database.");

		rc = fread(row->name, sizeof(char)*conn->db->max_data, 1, conn->file);
		if (rc != 1)
			die(conn, "Failed to load database.");

		rc = fread(&row->id, sizeof(int), 1, conn->file);
		if (rc != 1)
			die(conn, "Failed to load database.");

		rc = fread(&row->set, sizeof(int), 1, conn->file);
		if (rc != 1)
			die(conn, "Failed to load database.");

		conn->db->rows[i] = row;

	}
}

struct Connection *Database_open(const char *filename, char mode, int data, int rows)
{
	struct Connection *conn = malloc(sizeof(struct Connection));
	if (!conn)
		die(conn, "Memory error");

	conn->db = malloc(sizeof(struct Database));
	if (!conn->db)
		die(conn, "Memory error");

	if (mode == 'c') {
		conn->file = fopen(filename, "w");
	} else {
		conn->file = fopen(filename, "r+");

		if (conn->file) {
			Database_load(conn, data, rows);
		}
	}

	if (!conn->file)
		die(conn, "Failed to open the file");

	return conn;
}

void Database_close(struct Connection *conn)
{
	int i = 0;

	if (conn) {
		if (conn->file)
			fclose(conn->file);
		if (conn->db) {
			if (conn->db->rows) {
				for (i=0;i < conn->db->max_rows; i++) {
					struct Address *row = conn->db->rows[i];
					if (row) {
						if (row->email)
							free(row->email);
						if (row->name)
							free(row->name);
						free(row);
					}
				}
				free(conn->db->rows);
			}
			free(conn->db);
		}
		free(conn);
	}
}

void Database_write(struct Connection *conn)
{
	rewind(conn->file);

	int rc = 0;
	int i = 0;

	rc = fwrite(&conn->db->max_data, sizeof(int), 1, conn->file);
	if (rc != 1)
		die(conn, "Failed to write database.");

	rc = fwrite(&conn->db->max_rows, sizeof(int), 1, conn->file);
	if (rc != 1)
		die(conn, "Failed to write database.");

	for (i=0; i < conn->db->max_rows;i++) {

		struct Address *row = conn->db->rows[i];
		int str_len = conn->db->max_data;

		rc = fwrite(row->email, sizeof(char)*str_len, 1, conn->file);
		if (rc != 1)
			die(conn, "Failed to write database.");

		rc = fwrite(row->name, sizeof(char)*str_len, 1, conn->file);
		if (rc != 1)
			die(conn, "Failed to write database.");

		rc = fwrite(&row->id, sizeof(int), 1, conn->file);
		if (rc != 1)
			die(conn, "Failed to write database.");

		rc = fwrite(&row->set, sizeof(int), 1, conn->file);
		if (rc != 1)
			die(conn, "Failed to write database.");

	}

	rc = fflush(conn->file);
	if (rc == -1)
		die(conn, "Cannot flush database.");
}

void Database_create(struct Connection *conn, int max_data, int max_rows)
{
	int i = 0;

	conn->db->max_data = max_data;
	conn->db->max_rows = max_rows;

	conn->db->rows = malloc(sizeof(struct Address *)*conn->db->max_rows);
	if (!conn->db->rows)
		die(conn, "Memory error");

	for (i = 0; i < conn->db->max_rows; i++) {
		struct Address *row = malloc(sizeof(struct Address));
		if (!row)
			die(conn, "Memory error");

		row->id = i;
		row->set = 0;
		row->email = calloc(1, sizeof(char)*conn->db->max_data);
		row->name = calloc(1, sizeof(char)*conn->db->max_data);

		if (!row->email || !row->name)
			die(conn, "Memory Error");

		conn->db->rows[i] = row;
	}
}

void Database_set(struct Connection *conn, int id, const char *name,
		const char *email)
{
	struct Address *addr = conn->db->rows[id];
	if (addr->set)
		die(conn, "Already set, delete it first");

	addr->set = 1;
	// WARNING: bug, read the "How To Break It" and fix this
	char *res = strncpy(addr->name, name, conn->db->max_data);
	// demonstrate the strncpy bug
	addr->name[conn->db->max_data - 1] = '\0';
	if (!res)
		die(conn, "Name copy failed");

	res = strncpy(addr->email, email, conn->db->max_data);
	if (!res)
		die(conn, "Email copy failed");
}

void Database_get(struct Connection *conn, int id)
{
	struct Address *addr = conn->db->rows[id];

	if (addr->set) {
		Address_print(addr);
	} else {
		die(conn, "ID is not set");
	}
}

void Database_delete(struct Connection *conn, int id)
{
	conn->db->rows[id]->set = 0;
}

void Database_list(struct Connection *conn)
{
	int i = 0;
	struct Database *db = conn->db;

	for (i = 0; i < conn->db->max_rows; i++) {
		struct Address *cur = db->rows[i];

		if (cur->set) {
			Address_print(cur);
		}
	}
}

void Database_find(struct Connection *conn, char *value)
{
	int i = 0;
	struct Database *db = conn->db;

	for (i = 0; i < db->max_rows; i++) {
		struct Address *cur = db->rows[i];

		if (cur->set) {
			if (strstr(cur->email, value)) {
				Address_print(cur);
			} else if (strstr(cur->name, value)) {
				Address_print(cur);
			}
		}
	}
}


int main(int argc, char *argv[])
{
	if (argc < 3)
		die(NULL, "USAGE: ex17 <dbfile> <action> [action params]");

	char *filename = argv[1];
	char action = argv[2][0];
	int id = 0;
	struct Connection *conn = NULL;

	if (action == 'c') {
		if (argc != 5)
			die(NULL, "need max_data, and max_rows");

		int rows = atoi(argv[3]);
		int data = atoi(argv[4]);

		conn = Database_open(filename, action, rows, data);
		Database_create(conn, atoi(argv[3]), atoi(argv[4]));
		Database_write(conn);
	} else {

		conn = Database_open(filename, action, -1, -1);
		if (action != 'l') {
			id = atoi(argv[3]);
			if (id >= conn->db->max_rows) {
				die(conn, "There's not that many records.");
			}
		}

		switch (action) {
			case 'g':
				if (argc != 4)
					die(conn, "Need an id to get");

				Database_get(conn, id);
				break;

			case 's':
				if (argc != 6)
					die(conn, "Need id, name, email to set");

				Database_set(conn, id, argv[4], argv[5]);
				Database_write(conn);
				break;

			case 'd':
				if (argc != 4)
					die(conn, "Need id to delete");

				Database_delete(conn, id);
				Database_write(conn);
				break;

			case 'l':
				Database_list(conn);
				break;

			case 'f':
				if (argc != 4)
					die(conn, "Need value to search for");
				Database_find(conn, argv[3]);
				break;

			default:
				die(conn, "Invalid action: c=create, g=get, s=set, d=del, l=list");
		}
	}

	Database_close(conn);

	return 0;
}
