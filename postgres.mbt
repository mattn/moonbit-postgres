///|

#external
type Connection

#external
type QueryResult

pub struct ConnectConfig {
  host : String
  port : Int
  database : String
  user : String
  password : String
}

pub enum PgError {
  ConnectionError(String)
  AuthError(String)
  QueryError(String)
  ProtocolError(String)
}

pub fn connect(config : ConnectConfig) -> Result[Connection, PgError] {
  let host = config.host
  let port = config.port.to_string()
  let database = config.database
  let user = config.user
  let password = config.password
  let conninfo = "host=" +
    host +
    " port=" +
    port +
    " dbname=" +
    database +
    " user=" +
    user +
    " password=" +
    password
  let conn = pg_connect_internal(conninfo)
  if pg_connection_is_null(conn) > 0 {
    let err = pg_error_message(conn)
    Err(PgError::ConnectionError(err))
  } else {
    Ok(conn)
  }
}

pub fn query(conn : Connection, sql : String) -> Result[QueryResult, PgError] {
  let result = pg_query_internal(conn, sql)
  if pg_result_is_null(result) > 0 {
    let err = pg_error_message(conn)
    Err(PgError::QueryError(err))
  } else {
    Ok(result)
  }
}

pub fn close(conn : Connection) -> Unit {
  pg_close_internal(conn)
}

pub fn rows(result : QueryResult) -> Array[Array[String]] {
  let n_rows = pg_ntuples(result)
  let n_cols = pg_nfields(result)
  let row_arrays = Array::make(n_rows, Array::make(n_cols, ""))
  for i = 0; i < n_rows; i = i + 1 {
    let row_arr = Array::make(n_cols, "")
    for j = 0; j < n_cols; j = j + 1 {
      let val = pg_getvalue(result, i, j)
      ignore(row_arr.set(j, val))
    }
    ignore(row_arrays.set(i, row_arr))
  }
  row_arrays
}

pub fn columns(result : QueryResult) -> Array[String] {
  let n_cols = pg_nfields(result)
  let cols = Array::make(n_cols, "")
  for i = 0; i < n_cols; i = i + 1 {
    let name = pg_fname(result, i)
    ignore(cols.set(i, name))
  }
  cols
}

pub fn affected_rows(result : QueryResult) -> Int {
  pg_cmdtuples(result).to_int()
}

pub fn free_result(result : QueryResult) -> Unit {
  pg_free_result(result)
}

extern "C" fn pg_connect_internal(conninfo : String) -> Connection = "pg_connect_internal"

extern "C" fn pg_query_internal(conn : Connection, sql : String) -> QueryResult = "pg_query_internal"

extern "C" fn pg_close_internal(conn : Connection) -> Unit = "pg_close_internal"

extern "C" fn pg_error_message(conn : Connection) -> String = "pg_error_message"

extern "C" fn pg_ntuples(result : QueryResult) -> Int = "pg_ntuples"

extern "C" fn pg_nfields(result : QueryResult) -> Int = "pg_nfields"

extern "C" fn pg_getvalue(result : QueryResult, row : Int, col : Int) -> String = "pg_getvalue"

extern "C" fn pg_fname(result : QueryResult, col : Int) -> String = "pg_fname"

extern "C" fn pg_cmdtuples(result : QueryResult) -> Int64 = "pg_cmdtuples"

extern "C" fn pg_free_result(result : QueryResult) -> Unit = "pg_free_result"

extern "C" fn pg_connection_is_null(conn : Connection) -> Int = "pg_connection_is_null"

extern "C" fn pg_result_is_null(result : QueryResult) -> Int = "pg_result_is_null"
