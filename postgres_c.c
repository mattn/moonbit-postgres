#include "moonbit.h"
#include <libpq-fe.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  PGconn *conn;
} Connection;

typedef struct {
  PGresult *result;
} QueryResult;

typedef struct {
  PGconn *conn;
  char *name;
} PreparedStatement;

void* pg_connect_internal(const char *conninfo) {
  PGconn *conn = PQconnectdb(conninfo);
  
  Connection *connection = malloc(sizeof(Connection));
  connection->conn = conn;
  return connection;
}

void* pg_query_internal(void *conn_ptr, const char *sql, int len) {
  Connection *connection = (Connection *)conn_ptr;
  PGresult *result = PQexec(connection->conn, sql);
  
  ExecStatusType status = PQresultStatus(result);
  
  if (status != PGRES_TUPLES_OK && status != PGRES_COMMAND_OK) {
    PQclear(result);
    return NULL;
  }
  
  QueryResult *query_result = malloc(sizeof(QueryResult));
  query_result->result = result;
  return query_result;
}

// The MoonBit side hands us the parameters already encoded: `values[i]` is the
// raw bytes, `lengths[i]` its length (-1 for a SQL NULL) and `formats[i]` is 0
// for text or 1 for binary. Nothing here owns the bytes, so nothing frees them.
typedef struct {
  const char **values;
  int *lengths;
  int *formats;
} ParamArrays;

static int build_params(ParamArrays *out, uint8_t **values, int32_t *lengths,
                        int32_t *formats, int n) {
  out->values = malloc(n * sizeof(char *));
  out->lengths = malloc(n * sizeof(int));
  out->formats = malloc(n * sizeof(int));
  if (n > 0 && (!out->values || !out->lengths || !out->formats)) {
    free((void *)out->values);
    free(out->lengths);
    free(out->formats);
    return 0;
  }
  for (int i = 0; i < n; i++) {
    if (lengths[i] < 0) {
      out->values[i] = NULL;
      out->lengths[i] = 0;
      out->formats[i] = 0;
    } else {
      out->values[i] = (const char *)values[i];
      out->lengths[i] = lengths[i];
      out->formats[i] = formats[i];
    }
  }
  return 1;
}

static void free_params(ParamArrays *p) {
  free((void *)p->values);
  free(p->lengths);
  free(p->formats);
}

static void* wrap_result(PGresult *result) {
  ExecStatusType status = PQresultStatus(result);
  if (status != PGRES_TUPLES_OK && status != PGRES_COMMAND_OK) {
    PQclear(result);
    return NULL;
  }
  QueryResult *query_result = malloc(sizeof(QueryResult));
  query_result->result = result;
  return query_result;
}

void* pg_prepare_internal(void *conn_ptr, const char *name, const char *sql) {
  Connection *connection = (Connection *)conn_ptr;
  PGresult *result = PQprepare(connection->conn, name, sql, 0, NULL);
  
  if (PQresultStatus(result) != PGRES_COMMAND_OK) {
    PQclear(result);
    return NULL;
  }
  
  PreparedStatement *stmt = malloc(sizeof(PreparedStatement));
  stmt->conn = connection->conn;
  stmt->name = strdup(name);
  PQclear(result);
  return stmt;
}

void* pg_execute_prepared_internal(void *stmt_ptr, uint8_t **values,
                                   int32_t *lengths, int32_t *formats, int32_t n) {
  PreparedStatement *stmt = (PreparedStatement *)stmt_ptr;

  ParamArrays p;
  if (!build_params(&p, values, lengths, formats, n)) {
    return NULL;
  }
  PGresult *result = PQexecPrepared(stmt->conn, stmt->name, n, p.values,
                                    p.lengths, p.formats, 0);
  free_params(&p);
  return wrap_result(result);
}

void pg_close_statement_internal(void *stmt_ptr) {
  PreparedStatement *stmt = (PreparedStatement *)stmt_ptr;
  if (stmt) {
    if (stmt->name) {
      free(stmt->name);
    }
    free(stmt);
  }
}

void pg_close_internal(void *conn_ptr) {
  Connection *connection = (Connection *)conn_ptr;
  if (connection && connection->conn) {
    PQfinish(connection->conn);
    free(connection);
  }
}

moonbit_string_t pg_error_message(void *conn_ptr) {
  Connection *connection = (Connection *)conn_ptr;
  if (!connection) {
    return moonbit_make_string(0, 0);
  }
  const char *err = PQerrorMessage(connection->conn);
  int len = strlen(err);
  moonbit_string_t result = moonbit_make_string_raw(len);
  for (int i = 0; i < len; i++) {
    result[i] = (uint16_t)err[i];
  }
  return result;
}

moonbit_string_t pg_statement_error(void *stmt_ptr) {
  PreparedStatement *stmt = (PreparedStatement *)stmt_ptr;
  if (!stmt || !stmt->conn) {
    return moonbit_make_string(0, 0);
  }
  const char *err = PQerrorMessage(stmt->conn);
  int len = strlen(err);
  moonbit_string_t result = moonbit_make_string_raw(len);
  for (int i = 0; i < len; i++) {
    result[i] = (uint16_t)err[i];
  }
  return result;
}

int32_t pg_ntuples(void *result_ptr) {
  QueryResult *query_result = (QueryResult *)result_ptr;
  return PQntuples(query_result->result);
}

int32_t pg_nfields(void *result_ptr) {
  QueryResult *query_result = (QueryResult *)result_ptr;
  return PQnfields(query_result->result);
}

moonbit_string_t pg_getvalue(void *result_ptr, int32_t row, int32_t col) {
  QueryResult *query_result = (QueryResult *)result_ptr;
  const char *val = PQgetvalue(query_result->result, row, col);
  int len = strlen(val);
  moonbit_string_t result = moonbit_make_string_raw(len);
  for (int i = 0; i < len; i++) {
    result[i] = (uint16_t)val[i];
  }
  return result;
}

moonbit_string_t pg_fname(void *result_ptr, int32_t col) {
  QueryResult *query_result = (QueryResult *)result_ptr;
  const char *name = PQfname(query_result->result, col);
  int len = strlen(name);
  moonbit_string_t result = moonbit_make_string_raw(len);
  for (int i = 0; i < len; i++) {
    result[i] = (uint16_t)name[i];
  }
  return result;
}

int64_t pg_cmdtuples(void *result_ptr) {
  QueryResult *query_result = (QueryResult *)result_ptr;
  const char *cmd = PQcmdTuples(query_result->result);
  if (cmd && strlen(cmd) > 0) {
    return atol(cmd);
  }
  return 0;
}

void pg_free_result(void *result_ptr) {
  QueryResult *query_result = (QueryResult *)result_ptr;
  if (query_result && query_result->result) {
    PQclear(query_result->result);
    free(query_result);
  }
}

int32_t pg_connection_is_null(void *conn_ptr) {
  return conn_ptr == NULL;
}

int32_t pg_result_is_null(void *result_ptr) {
  return result_ptr == NULL;
}

int32_t pg_statement_is_null(void *stmt_ptr) {
  return stmt_ptr == NULL;
}

moonbit_string_t pg_get_env(void *name_bytes, int name_len) {
  // Bytes in MoonBit is a pointer to byte array data
  unsigned char *bytes = (unsigned char *)name_bytes;
  if (!bytes) {
    return moonbit_make_string(0, 0);
  }
  
  // string_to_c_bytes adds null terminator, so we can use it directly
  const char *value = getenv((const char *)bytes);
  
  if (!value) {
    return moonbit_make_string(0, 0);
  }
  
  int value_len = strlen(value);
  moonbit_string_t result = moonbit_make_string_raw(value_len);
  for (int i = 0; i < value_len; i++) {
    result[i] = (uint16_t)value[i];
  }
  return result;
}

void* pg_execute_internal(void *conn_ptr, const char *sql, uint8_t **values,
                          int32_t *lengths, int32_t *formats, int32_t n) {
  Connection *connection = (Connection *)conn_ptr;

  ParamArrays p;
  if (!build_params(&p, values, lengths, formats, n)) {
    return NULL;
  }
  PGresult *result = PQexecParams(connection->conn, sql, n, NULL, p.values,
                                  p.lengths, p.formats, 0);
  free_params(&p);
  return wrap_result(result);
}
