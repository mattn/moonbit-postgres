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
pub enum PgError {
  ConnectionError(String)
  AuthError(String)
  QueryError(String)
  ProtocolError(String)
}

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
pub fn Connection::query_and_fetch(self : Connection, sql : String) -> Result[Array[Array[String]], PgError] {
  match self.query(sql) {
    Ok(result) => {
      let rows = result.rows()
      result.free()
      Ok(rows)
    }
    Err(e) => Err(e)
  }
}

pub fn query_and_fetch(conn : Connection, sql : String) -> Result[Array[Array[String]], PgError] {
  self.query_and_fetch(sql)
}

///|
pub fn Connection::execute_and_fetch(self : Connection, sql : String, params : Array[Value]) -> Result[Array[Array[String]], PgError] {
  match self.execute(sql, params) {
    Ok(result) => {
      let rows = result.rows()
      result.free()
      Ok(rows)
    }
    Err(e) => Err(e)
  }
}

pub fn execute_and_fetch(conn : Connection, sql : String, params : Array[Value]) -> Result[Array[Array[String]], PgError] {
  self.execute_and_fetch(sql, params)
}

///|
pub fn PreparedStatement::execute_and_fetch(self : PreparedStatement, params : Array[Value]) -> Result[Array[Array[String]], PgError] {
  match self.execute(params) {
    Ok(result) => {
      let rows = result.rows()
      result.free()
      Ok(rows)
    }
    Err(e) => Err(e)
  }
}

pub fn execute_prepared_and_fetch(stmt : PreparedStatement, params : Array[Value]) -> Result[Array[Array[String]], PgError] {
  self.execute_and_fetch(params)
}

pub fn pg_connect(conninfo : String) -> Result[Connection, PgError] {
  let conn = pg_connect_internal(string_to_c_bytes(conninfo))
  let err = pg_error_message(conn)
  if err.length() > 0 {
    pg_close_internal(conn)
    Err(PgError::ConnectionError(err))
  } else {
    Ok(conn)
  }
}

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
pub fn Connection::query(self : Connection, sql : String) -> Result[QueryResult, PgError] {
  let result = pg_query_internal(self, string_to_c_bytes(sql))
  if pg_result_is_null(result) > 0 {
    let err = pg_error_message(self)
    Err(PgError::QueryError(err))
  } else {
    Ok(result)
  }
}

///|
pub fn query(conn : Connection, sql : String) -> Result[QueryResult, PgError] {
  self.query(sql)
}

///|
pub fn Connection::execute(self : Connection, sql : String, params : Array[Value]) -> Result[QueryResult, PgError] {
  let result = pg_execute_internal(self, string_to_c_bytes(sql), params)
  if pg_result_is_null(result) > 0 {
    let err = pg_error_message(self)
    Err(PgError::QueryError(err))
  } else {
    Ok(result)
  }
}

///|
pub fn execute(conn : Connection, sql : String, params : Array[Value]) -> Result[QueryResult, PgError] {
  self.execute(sql, params)
}

fn parse_parameter_placeholders(sql : String) -> Array[Int] {
  let mut result = Array::make(0, 0)
  let mut i = 0
  while i < sql.length() {
    if i + 1 < sql.length() && sql[i] == '$' {
      let mut j = i + 1
      let mut num = 0
      while j < sql.length() && sql[j].to_int() >= '0'.to_int() && sql[j].to_int() <= '9'.to_int() {
        num = num * 10 + (sql[j].to_int() - '0'.to_int())
        j = j + 1
      }
      if num > 0 {
        result.push(num - 1)
      }
      i = j
    } else {
      i = i + 1
    }
  }
  result
}

fn parse_parameter_placeholders(sql : String) -> Array[Int] {
  let mut result = Array::make(0, 0)
  let mut i = 0
  while i < sql.length() {
    if i + 1 < sql.length() && sql[i] == '$' {
      let mut j = i + 1
      let mut num = 0
      while j < sql.length() && sql[j].to_int() >= '0'.to_int() && sql[j].to_int() <= '9'.to_int() {
        num = num * 10 + (sql[j].to_int() - '0'.to_int())
        j = j + 1
      }
      if num > 0 {
        result.push(num - 1)
      }
      i = j
    } else {
      i = i + 1
    }
  }
  result
}
      if num > 0 && num - 1 < params.length() {
        let param = params[num - 1]
        match param {
          Value::String(param_str) => result = result + "'" + param_str + "'"
          Value::Int(n) => result = result + n.to_string()
          Value::Int64(n) => result = result + n.to_string()
          Value::Float(f) => result = result + f.to_string()
          Value::Bool(b) => result = result + (if b { "true" } else { "false" })
          Value::Null => result = result + "NULL"
          Value::Bytes(b) => result = result + "\\x"
        }
        param_index = param_index + 1
      }
      i = j
    } else {
      result = result + sql[i].to_string()
      i = i + 1
    }
  }
  result
}

///|
pub fn Connection::prepare(self : Connection, name : String, sql : String) -> Result[PreparedStatement, PgError] {
  let stmt = pg_prepare_internal(self, string_to_c_bytes(name), string_to_c_bytes(sql))
  if pg_statement_is_null(stmt) > 0 {
    let err = pg_error_message(self)
    Err(PgError::QueryError(err))
  } else {
    Ok(stmt)
  }
}

///|
pub fn prepare(conn : Connection, name : String, sql : String) -> Result[PreparedStatement, PgError] {
  self.prepare(name, sql)
}

///|
pub fn PreparedStatement::execute(self : PreparedStatement, params : Array[Value]) -> Result[QueryResult, PgError] {
  let result = pg_execute_prepared_internal(self, params)
  if pg_result_is_null(result) > 0 {
    let err = pg_statement_error(self)
    Err(PgError::QueryError(err))
  } else {
    Ok(result)
  }
}

///|
pub fn execute_prepared(stmt : PreparedStatement, params : Array[Value]) -> Result[QueryResult, PgError] {
  self.execute(params)
}

///|
pub fn PreparedStatement::close(self : PreparedStatement) -> Unit {
  pg_close_statement_internal(self)
}

// Prepared statements are useful when executing the same query multiple times.
// For simple one-time queries, just use execute().
// Example usage:
//   let stmt = prepare(conn, "my_stmt", "SELECT * FROM users WHERE id = $1")?
//   let rows1 = execute_prepared_and_fetch(stmt, [@postgres.Value::Int(1)])
//   let rows2 = execute_prepared_and_fetch(stmt, [@postgres.Value::Int(2)])
//   close_statement(stmt)

///|
pub fn Connection::close(self : Connection) -> Unit {
  pg_close_internal(self)
}

pub fn query_and_fetch(conn : Connection, sql : String) -> Result[Array[Array[String]], PgError] {
  match query(conn, sql) {
    Ok(result) => {
      let rows = rows(result)
      free_result(result)
      Ok(rows)
    }
    Err(e) => Err(e)
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
pub fn rows(result : QueryResult) -> Array[Array[String]] {
  self.rows()
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
pub fn columns(result : QueryResult) -> Array[String] {
  self.columns()
}

///|
pub fn QueryResult::affected_rows(self : QueryResult) -> Int {
  pg_cmdtuples(self).to_int()
}

///|
pub fn affected_rows(result : QueryResult) -> Int {
  self.affected_rows()
}

///|
pub fn QueryResult::free(self : QueryResult) -> Unit {
  pg_free_result(self)
}

///|
pub fn free_result(result : QueryResult) -> Unit {
  self.free()
}

///|
extern "C" fn pg_connect_internal(conninfo : Bytes) -> Connection = "pg_connect_internal"

///|
extern "C" fn pg_query_internal(conn : Connection, sql : Bytes) -> QueryResult = "pg_query_internal"

///|
extern "C" fn pg_execute_internal(conn : Connection, sql : Bytes, params : Array[Value]) -> QueryResult = "pg_execute_internal"

///|
extern "C" fn pg_prepare_internal(conn : Connection, name : Bytes, sql : Bytes) -> PreparedStatement = "pg_prepare_internal"

///|
extern "C" fn pg_execute_prepared_internal(stmt : PreparedStatement, params : Array[Value]) -> QueryResult = "pg_execute_prepared_internal"

///|
extern "C" fn pg_close_statement_internal(stmt : PreparedStatement) -> Unit = "pg_close_statement_internal"

///|
extern "C" fn pg_close_internal(conn : Connection) -> Unit = "pg_close_internal"

///|
extern "C" fn pg_error_message(conn : Connection) -> String = "pg_error_message"

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
