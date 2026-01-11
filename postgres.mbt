///|
#external
type Connection

///|
#external
type QueryResult

///|
#external
type PreparedStatement

///|

///|
pub enum Value {
  Null
  Bool(Bool)
  Int(Int)
  Int64(Int64)
  Float(Float)
  String(String)
  Bytes(Bytes)
}

///|
pub enum PgError {
  ConnectionError(String)
  AuthError(String)
  QueryError(String)
  ProtocolError(String)
}

///|
pub fn PgError::to_string(self : PgError) -> String {
  match self {
    ConnectionError(s) => "ConnectionError(" + s + ")"
    AuthError(s) => "AuthError(" + s + ")"
    QueryError(s) => "QueryError(" + s + ")"
    ProtocolError(s) => "ProtocolError(" + s + ")"
  }
}

///|
pub fn connect(conninfo : String) -> Result[Connection, PgError] {
  let conn = pg_connect_internal(string_to_c_bytes(conninfo))
  let err = pg_error_message(conn)
  if err.length() > 0 {
    pg_close_internal(conn)
    Err(PgError::ConnectionError(err))
  } else {
    Ok(conn)
  }
}

///|
fn string_to_c_bytes(s : String) -> Bytes {
  let arr = Array::make(0, (0 : Byte))
  for i = 0; i < s.length(); i = i + 1 {
    let c = s[i]
    let hi = (c >> 8).to_byte()
    let lo = c.to_byte()
    if hi != (0 : Byte) {
      arr.push(hi)
    }
    arr.push(lo)
  }
  arr.push((0 : Byte))
  Bytes::from_array(arr)
}

///|
pub fn Connection::query(
  self : Connection,
  sql : String,
) -> Result[QueryResult, PgError] {
  let result = pg_query_internal(self, string_to_c_bytes(sql))
  if pg_result_is_null(result) > 0 {
    let err = pg_error_message(self)
    Err(PgError::QueryError(err))
  } else {
    Ok(result)
  }
}

///|
pub fn Connection::execute(
  self : Connection,
  sql : String,
  params : Array[Value],
) -> Result[QueryResult, PgError] {
  let result = pg_execute_internal(self, string_to_c_bytes(sql), params)
  if pg_result_is_null(result) > 0 {
    let err = pg_error_message(self)
    Err(PgError::QueryError(err))
  } else {
    Ok(result)
  }
}

///|
pub fn QueryResult::rows(self : QueryResult) -> Array[Array[String]] {
  let n_rows = pg_ntuples(self)
  let n_cols = pg_nfields(self)
  let row_arrays = Array::make(n_rows, Array::make(n_cols, ""))
  for i = 0; i < n_rows; i = i + 1 {
    let row_arr = Array::make(n_cols, "")
    for j = 0; j < n_cols; j = j + 1 {
      let val = pg_getvalue(self, i, j)
      ignore(row_arr.set(j, val))
    }
    ignore(row_arrays.set(i, row_arr))
  }
  row_arrays
}

///|
pub fn QueryResult::columns(self : QueryResult) -> Array[String] {
  let n_cols = pg_nfields(self)
  let cols = Array::make(n_cols, "")
  for i = 0; i < n_cols; i = i + 1 {
    let name = pg_fname(self, i)
    ignore(cols.set(i, name))
  }
  cols
}

///|
pub fn QueryResult::affected_rows(self : QueryResult) -> Int {
  pg_cmdtuples(self).to_int()
}

///|
pub fn QueryResult::free(self : QueryResult) -> Unit {
  pg_free_result(self)
}

///|
pub fn Connection::prepare(
  self : Connection,
  name : String,
  sql : String,
) -> Result[PreparedStatement, PgError] {
  let stmt = pg_prepare_internal(
    self,
    string_to_c_bytes(name),
    string_to_c_bytes(sql),
  )
  if pg_statement_is_null(stmt) > 0 {
    let err = pg_error_message(self)
    Err(PgError::QueryError(err))
  } else {
    Ok(stmt)
  }
}

///|
pub fn PreparedStatement::execute(
  self : PreparedStatement,
  params : Array[Value],
) -> Result[QueryResult, PgError] {
  let result = pg_execute_prepared_internal(self, params)
  if pg_result_is_null(result) > 0 {
    let err = pg_statement_error(self)
    Err(PgError::QueryError(err))
  } else {
    Ok(result)
  }
}

///|
pub fn PreparedStatement::close(self : PreparedStatement) -> Unit {
  pg_close_statement_internal(self)
}

///|
pub fn Connection::close(self : Connection) -> Unit {
  pg_close_internal(self)
}

///|
extern "C" fn pg_connect_internal(conninfo : Bytes) -> Connection = "pg_connect_internal"

///|
extern "C" fn pg_query_internal(conn : Connection, sql : Bytes) -> QueryResult = "pg_query_internal"

///|
extern "C" fn pg_execute_internal(
  conn : Connection,
  sql : Bytes,
  params : Array[Value],
) -> QueryResult = "pg_execute_internal"

///|
extern "C" fn pg_prepare_internal(
  conn : Connection,
  name : Bytes,
  sql : Bytes,
) -> PreparedStatement = "pg_prepare_internal"

///|
extern "C" fn pg_execute_prepared_internal(
  stmt : PreparedStatement,
  params : Array[Value],
) -> QueryResult = "pg_execute_prepared_internal"

///|
extern "C" fn pg_close_statement_internal(stmt : PreparedStatement) -> Unit = "pg_close_statement_internal"

///|
extern "C" fn pg_close_internal(conn : Connection) -> Unit = "pg_close_internal"

///|
extern "C" fn pg_error_message(conn : Connection) -> String = "pg_error_message"

///|
extern "C" fn pg_statement_error(stmt : PreparedStatement) -> String = "pg_statement_error"

///|
extern "C" fn pg_ntuples(result : QueryResult) -> Int = "pg_ntuples"

///|
extern "C" fn pg_nfields(result : QueryResult) -> Int = "pg_nfields"

///|
extern "C" fn pg_getvalue(result : QueryResult, row : Int, col : Int) -> String = "pg_getvalue"

///|
extern "C" fn pg_fname(result : QueryResult, col : Int) -> String = "pg_fname"

///|
extern "C" fn pg_cmdtuples(result : QueryResult) -> Int64 = "pg_cmdtuples"

///|
extern "C" fn pg_free_result(result : QueryResult) -> Unit = "pg_free_result"

///|
extern "C" fn pg_connection_is_null(conn : Connection) -> Int = "pg_connection_is_null"

///|
extern "C" fn pg_result_is_null(result : QueryResult) -> Int = "pg_result_is_null"

///|
extern "C" fn pg_statement_is_null(stmt : PreparedStatement) -> Int = "pg_statement_is_null"
