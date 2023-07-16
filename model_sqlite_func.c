#define __STDC_WANT_LIB_EXT1__ 1
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "ross.h"
#include "sqlite3.h"
#include "model.h"
#include "log.h"


void db_init(void ** _db, void * lp, void * mem_pool_ptr, size_t mem_pool_size, const char * db_filename, const char * init_script_filename);
void db_exec_from_file(void * db, void * lp, const char * filename);
int db_exec(void * db, void * lp, const char * sql);

static int db_exec_id(void * db, void * lp, const char * sql, int * row_id);


void db_init(void ** _db, void * lp, void * mem_pool_ptr, size_t mem_pool_size, const char * dv_filename, const char * init_script_filename) {
	int rc = sqlite3_open(dv_filename, (struct sqlite3 **) _db);

	if (rc != SQLITE_OK) {
		log_error(lp, "Error initializing SQLite %d\n",  rc);
		exit(3);
	}

	sqlite3_config(SQLITE_CONFIG_HEAP, mem_pool_ptr, mem_pool_size, 512);

	struct sqlite3 * db = (struct sqlite3 *) *_db;

	sqlite3_exec(db, "PRAGMA journal_mode = MEMORY", NULL, NULL, NULL);
	db_exec_from_file(db, lp, init_script_filename);
}


void db_exec_from_file(void * db, void * lp, const char * filename) {
 	FILE *fp = fopen(filename, "r");
    if (!fp) {
        log_error(lp, "Error in opening file %s", filename);
        exit(1);
    }

    fseek(fp, 0L, SEEK_END);
    
    int file_size = ftell(fp);

    fseek(fp, 0L, SEEK_SET);

    char * file_content = (char*)malloc(file_size+1);

    if (!file_content) {
    	log_error(lp, "Can't allocate memory to read file %s", filename);
    	exit(3);    	
    }
    if (fread(file_content, sizeof(char), file_size, fp) != file_size) {
    	log_error(lp, "Error in reading file %s", filename);
    	exit(3);
    }

    file_content[file_size] = 0;

    fclose(fp);

    const char delim[2] = ";";
 	char * sql_query = strtok(file_content, delim);

 	while(sql_query) {   
    	db_exec(db, lp, sql_query);
    	sql_query = strtok(NULL, delim);
    }

    free(file_content);
}


int db_exec_id(void * db, void * lp, const char * sql, int * row_id) {
	int rc = sqlite3_exec((struct sqlite3*) db, sql, NULL, NULL, NULL);
	if (rc != SQLITE_OK) {
		log_error(lp, "Error %d in sql query '%s'\n", rc, sql );
		exit(3);
	}
	if (row_id) {
		*row_id = sqlite3_last_insert_rowid(db);
	}
	return rc;
} 


int db_exec(void *db, void * lp, const char * sql) {
	return db_exec_id(db, lp, sql, NULL);
} 


sqlite3_stmt * db_prepare_select(void * db, void * lp, const char * sql, int * step) {
	sqlite3_stmt *stmp;
	int rc = sqlite3_prepare_v2(db, sql, -1, &stmp, 0 );

	log_debug(lp, sql);
	if (rc != SQLITE_OK) {
		log_error(lp, "Error %d in sql query '%s'\n", rc, sql );
		exit(3);
	}

	*step = sqlite3_step(stmp);

	if (*step != SQLITE_ROW && *step != SQLITE_DONE) {
		log_error(lp, "Error %d when fetching data by query '%s'\n", step, sql );
		exit(3);		
	}
	return stmp;
}


 sqlite3_stmt * db_prepare_select_v2(void * db, void * lp, const char * sql, int * step) {
	sqlite3_stmt *stmp;
	int rc = sqlite3_prepare_v2(db, sql, -1, &stmp, 0 );

	if (rc != SQLITE_OK && rc != SQLITE_DONE) {
		log_error(lp, "Error %d in sql query '%s'\n", rc, sql );
		exit(3);
	}
	*step = rc;
	return stmp;
 }


int db_get_int_from_sql(void * db, void * lp, const char * sql) {
	int step;
	sqlite3_stmt *stmp = db_prepare_select(db, lp, sql, &step);
	int res = step != SQLITE_DONE ? sqlite3_column_int(stmp, 0) : -1;
	sqlite3_finalize(stmp);
	return res;
}


sqlite3_int64 db_get_int64_from_sql(void * db, void * lp, const char * sql) {
	int step;
	sqlite3_stmt *stmp = db_prepare_select(db, lp, sql, &step);
	sqlite3_int64 res = step != SQLITE_DONE ? sqlite3_column_int64(stmp, 0) : -1;
	sqlite3_finalize(stmp);
	return res;
}


/* Model functions here */
void db_dec_action(void * db, void * lp, int id) {
	char sql[255] = "";
	snprintf(sql, sizeof(sql), "update `sample_model` "
	    "set `count` = `count` - 1 "
		"where `id` = %d", id);
	db_exec(db, lp, sql);
}


void db_inc_action(void * db, void * lp, int id) {
	char sql[255] = "";
	snprintf(sql, sizeof(sql), "update `sample_model` "
	    "set `count` = `count` + 1 "
		"where `id` = %d", id);
	db_exec(db, lp, sql);
}


void db_print_row(void * db, void * lp, int id) {
	int step;
	static sqlite3_stmt * query_stmt = NULL;
	
	if (!query_stmt) {
		char sql[255] = "";
		snprintf(sql, sizeof(sql), "select `id`, `count`, `description` from `sample_model` where `id`=?");
		query_stmt = db_prepare_select_v2(db, lp, sql, &step);
	}

	sqlite3_bind_int(query_stmt, 1, id);
	step = sqlite3_step(query_stmt);

	while(step == SQLITE_ROW) {
		int _id = sqlite3_column_int(query_stmt, 0);
		int _count = sqlite3_column_int(query_stmt, 1);
		const unsigned char * _desc = sqlite3_column_text(query_stmt, 2);
		log_info(lp, "id = %d, count = %d, description = %s", _id, _count, _desc);
		step = sqlite3_step(query_stmt);
	}
	sqlite3_reset(query_stmt);
}