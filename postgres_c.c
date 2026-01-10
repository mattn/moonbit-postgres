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

void* pg_connect_internal(const char *conninfo) {
  PGconn *conn = PQconnectdb(conninfo);
  
  if (PQstatus(conn) != CONNECTION_OK) {
    const char *err = PQerrorMessage(conn);
    PQfinish(conn);
    return NULL;
  }
  
  Connection *connection = malloc(sizeof(Connection));
  connection->conn = conn;
  return connection;
}

void* pg_query_internal(void *conn_ptr, const char *sql) {
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
