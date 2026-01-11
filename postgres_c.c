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

void* pg_execute_internal(void *conn_ptr, const char *sql, int sql_len, void *params, int param_count) {
  Connection *connection = (Connection *)conn_ptr;
  
  char **param_values = malloc(param_count * sizeof(char *));
  int *param_lengths = malloc(param_count * sizeof(int));
  int *param_formats = malloc(param_count * sizeof(int));
  
  for (int i = 0; i < param_count; i++) {
    moonbit_value_t *param = (moonbit_value_t *)(((uint64_t *)params)[i]);
    
    switch (param->tag) {
      case 0: { // Null
        param_values[i] = NULL;
        param_lengths[i] = 0;
        param_formats[i] = 1; // text
        break;
      }
      case 1: { // Bool
        if (param->payload_bool) {
          param_values[i] = "t";
          param_lengths[i] = 1;
        } else {
          param_values[i] = "f";
          param_lengths[i] = 1;
        }
        param_formats[i] = 1;
        break;
      }
      case 2: { // Int
        char buf[32];
        int len = snprintf(buf, sizeof(buf), "%d", (int)param->payload_int);
        param_values[i] = strdup(buf);
        param_lengths[i] = len;
        param_formats[i] = 1;
        break;
      }
      case 3: { // Int64
        char buf[64];
        int len = snprintf(buf, sizeof(buf), "%lld", (long long)param->payload_int64);
        param_values[i] = strdup(buf);
        param_lengths[i] = len;
        param_formats[i] = 1;
        break;
      }
      case 4: { // Float
        char buf[64];
        int len = snprintf(buf, sizeof(buf), "%f", (double)param->payload_float);
        param_values[i] = strdup(buf);
        param_lengths[i] = len;
        param_formats[i] = 1;
        break;
      }
      case 5: { // String
        uint16_t len = ((moonbit_string_t *)param)->length;
        param_values[i] = malloc(len + 1);
        for (int j = 0; j < len; j++) {
          ((char *)param_values[i])[j] = (char)((moonbit_string_t *)param)->payload[j];
        }
        ((char *)param_values[i])[len] = '\0';
        param_lengths[i] = len;
        param_formats[i] = 1;
        break;
      }
      case 6: { // Bytes
        moonbit_bytes_t *bytes = (moonbit_bytes_t *)param;
        param_values[i] = malloc(bytes->length);
        memcpy(param_values[i], bytes->payload, bytes->length);
        param_lengths[i] = bytes->length;
        param_formats[i] = 1; // binary
        break;
      }
      default:
        param_values[i] = NULL;
        param_lengths[i] = 0;
        param_formats[i] = 1;
    }
  }
  
  PGresult *result = PQexecParams(connection->conn, sql, param_count, param_values, NULL, param_lengths, param_formats, 1);
  
  for (int i = 0; i < param_count; i++) {
    if (param_values[i]) {
      free(param_values[i]);
    }
  }
  free(param_values);
  free(param_lengths);
  free(param_formats);
  
  ExecStatusType status = PQresultStatus(result);
  
  if (status != PGRES_TUPLES_OK && status != PGRES_COMMAND_OK) {
    PQclear(result);
    return NULL;
  }
  
  QueryResult *query_result = malloc(sizeof(QueryResult));
  query_result->result = result;
  return query_result;
}

void* pg_prepare_internal(void *conn_ptr, const char *name, int name_len, const char *sql, int sql_len) {
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

void* pg_execute_prepared_internal(void *stmt_ptr, void *params, int param_count) {
  PreparedStatement *stmt = (PreparedStatement *)stmt_ptr;
  
  char **param_values = malloc(param_count * sizeof(char *));
  int *param_lengths = malloc(param_count * sizeof(int));
  int *param_formats = malloc(param_count * sizeof(int));
  
  for (int i = 0; i < param_count; i++) {
    moonbit_value_t *param = (moonbit_value_t *)(((uint64_t *)params)[i]);
    
    switch (param->tag) {
      case 0: { // Null
        param_values[i] = NULL;
        param_lengths[i] = 0;
        param_formats[i] = 1;
        break;
      }
      case 1: { // Bool
        if (param->payload_bool) {
          param_values[i] = "t";
          param_lengths[i] = 1;
        } else {
          param_values[i] = "f";
          param_lengths[i] = 1;
        }
        param_formats[i] = 1;
        break;
      }
      case 2: { // Int
        char buf[32];
        int len = snprintf(buf, sizeof(buf), "%d", (int)param->payload_int);
        param_values[i] = strdup(buf);
        param_lengths[i] = len;
        param_formats[i] = 1;
        break;
      }
      case 3: { // Int64
        char buf[64];
        int len = snprintf(buf, sizeof(buf), "%lld", (long long)param->payload_int64);
        param_values[i] = strdup(buf);
        param_lengths[i] = len;
        param_formats[i] = 1;
        break;
      }
      case 4: { // Float
        char buf[64];
        int len = snprintf(buf, sizeof(buf), "%f", (double)param->payload_float);
        param_values[i] = strdup(buf);
        param_lengths[i] = len;
        param_formats[i] = 1;
        break;
      }
      case 5: { // String
        uint16_t len = ((moonbit_string_t *)param)->length;
        param_values[i] = malloc(len + 1);
        for (int j = 0; j < len; j++) {
          ((char *)param_values[i])[j] = (char)((moonbit_string_t *)param)->payload[j];
        }
        ((char *)param_values[i])[len] = '\0';
        param_lengths[i] = len;
        param_formats[i] = 1;
        break;
      }
      case 6: { // Bytes
        moonbit_bytes_t *bytes = (moonbit_bytes_t *)param;
        param_values[i] = malloc(bytes->length);
        memcpy(param_values[i], bytes->payload, bytes->length);
        param_lengths[i] = bytes->length;
        param_formats[i] = 1;
        break;
      }
      default:
        param_values[i] = NULL;
        param_lengths[i] = 0;
        param_formats[i] = 1;
    }
  }
  
  PGresult *result = PQexecPrepared(stmt->conn, stmt->name, param_count, param_values, NULL, param_lengths, param_formats, 1);
  
  for (int i = 0; i < param_count; i++) {
    if (param_values[i]) {
      free(param_values[i]);
    }
  }
  free(param_values);
  free(param_lengths);
  free(param_formats);
  
  ExecStatusType status = PQresultStatus(result);
  
  if (status != PGRES_TUPLES_OK && status != PGRES_COMMAND_OK) {
    PQclear(result);
    return NULL;
  }
  
  QueryResult *query_result = malloc(sizeof(QueryResult));
  query_result->result = result;
  return query_result;
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

int32_t pg_statement_is_null(void *stmt_ptr) {
  return stmt_ptr == NULL;
}

int32_t pg_result_is_null(void *result_ptr) {
  return result_ptr == NULL;
}
