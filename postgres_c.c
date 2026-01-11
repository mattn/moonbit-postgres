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
    void *param_val = ((void **)params)[i];
    uint8_t tag = ((uint8_t *)param_val)[0];
    
    switch (tag) {
      case 0: { // Null
        param_values[i] = NULL;
        param_lengths[i] = 0;
        param_formats[i] = 1;
        break;
      }
      case 1: { // Bool
        uint8_t b = ((uint8_t *)param_val)[1];
        if (b) {
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
        int n = *(int *)((char *)param_val + 1);
        char buf[32];
        int len = snprintf(buf, sizeof(buf), "%d", n);
        param_values[i] = strdup(buf);
        param_lengths[i] = len;
        param_formats[i] = 1;
        break;
      }
      case 3: { // Int64
        long long n = *(long long *)((char *)param_val + 1);
        char buf[64];
        int len = snprintf(buf, sizeof(buf), "%lld", n);
        param_values[i] = strdup(buf);
        param_lengths[i] = len;
        param_formats[i] = 1;
        break;
      }
      case 4: { // Float
        double f = *(double *)((char *)param_val + 1);
        char buf[64];
        int len = snprintf(buf, sizeof(buf), "%f", f);
        param_values[i] = strdup(buf);
        param_lengths[i] = len;
        param_formats[i] = 1;
        break;
      }
      case 5: { // String
        int16_t len = *(int16_t *)((char *)param_val + 1);
        char *str = (char *)((char *)param_val + 1 + 2);
        param_values[i] = malloc(len + 1);
        for (int j = 0; j < len; j++) {
          ((char *)param_values[i])[j] = str[j];
        }
        ((char *)param_values[i])[len] = '\0';
        param_lengths[i] = len;
        param_formats[i] = 1;
        break;
      }
      case 6: { // Bytes
        int16_t len = *(int16_t *)((char *)param_val + 1);
        char *bytes = (char *)((char *)param_val + 1 + 2);
        param_values[i] = malloc(len);
        memcpy(param_values[i], bytes, len);
        param_lengths[i] = len;
        param_formats[i] = 1;
        break;
      }
      default:
        param_values[i] = NULL;
        param_lengths[i] = 0;
        param_formats[i] = 1;
        break;
      }
      }
      
      PGresult *result = PQexecPrepared(stmt->conn, stmt->name, param_count, (const char * const *)param_values, param_lengths, param_formats, 1);
  
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

void* pg_execute_internal(void *conn_ptr, const char *sql, int sql_len, int param_count, void *param_array) {
  Connection *connection = (Connection *)conn_ptr;
  
  char **param_values = malloc(param_count * sizeof(char *));
  int *param_lengths = malloc(param_count * sizeof(int));
  int *param_formats = malloc(param_count * sizeof(int));

  for (int i = 0; i < param_count; i++) {
    // MoonBit Value enum: access array of Value structs
    unsigned char *value_ptr = ((unsigned char *)param_array) + (i * 16);
    uint8_t tag = value_ptr[0];

    switch (tag) {
      case 0: { // Null
        param_values[i] = NULL;
        param_lengths[i] = 0;
        param_formats[i] = 1;
        break;
      }
      case 1: { // Bool
        uint8_t b = value_ptr[1];
        if (b) {
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
        int n = *(int *)(value_ptr + 1);
        char buf[32];
        int len = snprintf(buf, sizeof(buf), "%d", n);
        param_values[i] = strdup(buf);
        param_lengths[i] = len;
        param_formats[i] = 1;
        break;
      }
      case 3: { // Int64
        long long n = *(long long *)(value_ptr + 1);
        char buf[64];
        int len = snprintf(buf, sizeof(buf), "%lld", n);
        param_values[i] = strdup(buf);
        param_lengths[i] = len;
        param_formats[i] = 1;
        break;
      }
      case 4: { // Float
        double f = *(double *)(value_ptr + 1);
        char buf[64];
        int len = snprintf(buf, sizeof(buf), "%f", f);
        param_values[i] = strdup(buf);
        param_lengths[i] = len;
        param_formats[i] = 1;
        break;
      }
      case 5: { // String
        int16_t len = *(int16_t *)(value_ptr + 1);
        char *str = (char *)(value_ptr + 1 + 2);
        param_values[i] = malloc(len + 1);
        for (int j = 0; j < len; j++) {
          ((char *)param_values[i])[j] = str[j];
        }
        ((char *)param_values[i])[len] = '\0';
        param_lengths[i] = len;
        param_formats[i] = 1;
        break;
      }
      case 6: { // Bytes
        int16_t len = *(int16_t *)(value_ptr + 1);
        char *bytes = (char *)(value_ptr + 1 + 2);
        param_values[i] = malloc(len);
        memcpy(param_values[i], bytes, len);
        param_lengths[i] = len;
        param_formats[i] = 1;
        break;
      }
      default:
        param_values[i] = NULL;
        param_lengths[i] = 0;
        param_formats[i] = 1;
    }
  }

  PGresult *result = PQexecParams(connection->conn, sql, param_count, NULL, (const char * const *)param_values, param_lengths, param_formats, 1);

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
