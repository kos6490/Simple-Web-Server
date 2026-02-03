#include <stdio.h>
#include <sqlite3.h>

int main() {
    sqlite3 *db;
    char *err_msg = 0;

    int rc = sqlite3_open("test.db", &db);

    if(rc != SQLITE_OK) {
        fprintf(stderr, "[Error] Cannot open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return 1;
    }

    printf("[Success] opened database!\n");

    char *sql = "CREATE TABLE IF NOT EXISTS Users(Id INTEGER PRIMARY KEY, Name TEXT);"
                "INSERT INTO Users VALUES(1, 'Manager');"
                "INSERT INTO Users VALUES(2, 'Developer');";

    rc = sqlite3_exec(db, sql, 0, 0, &err_msg);

    if (rc != SQLITE_OK) {
        fprintf(stderr, "[Error] SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
    } else {
        printf("[Success] Table created and data inserted!\n");
    }

    sqlite3_close(db);
    return 0;
}