# mattn/postgres

A PostgreSQL client library for MoonBit using libpq.

## Features

- Connection management with PostgreSQL servers
- Query execution
- Result parsing (rows, columns, affected rows)
- Error handling for connection and query failures

## Usage

### 1. Import the Package

Simply add the dependency to your `moon.pkg.json`:

```json
{
  "is-main": true,
  "import": [
    {
      "path": "mattn/postgres",
      "alias": "postgres"
    }
  ],
  "supported-targets": ["native"],
  "targets": {
    "main.mbt": ["native"]
  }
}
```

**Note**: Starting from v0.8.3, the PostgreSQL `libpq` library linking is automatically configured through the pre-build script. If you're using an older version, you may need to explicitly add:

```json
"link": {
  "native": {
    "cc-link-flags": "-lpq"
  }
}
```

### 2. Use the Library

```moonbit
use postgres

let config = postgres::ConnectConfig::new(
  "localhost",
  5432,
  "mydb",
  "postgres",
  "password"
)

match postgres::connect(config) {
  Ok(conn) => {
    match postgres::query(conn, "SELECT * FROM users") {
      Ok(result) => {
        let rows = postgres::rows(result)
        let cols = postgres::columns(result)
        // Process results
        postgres::free_result(result)
      }
      Err(e) => {
        // Handle query error
      }
    }
    postgres::close(conn)
  }
  Err(e) => {
    // Handle connection error
  }
}
```

## Requirements

- PostgreSQL development libraries (libpq-dev) installed on your system
- On Ubuntu/Debian: `sudo apt-get install libpq-dev`
- On macOS: `brew install postgresql`
- On Fedora/RHEL: `sudo dnf install postgresql-devel`

## Building

This package uses C FFI to bind with libpq. Build with native target:

```bash
moon build --target native
```

## API Reference

### Connection Methods

#### `conn.close() -> Unit`

Close the database connection.

#### `conn.query(sql: String) -> Result[QueryResult, PgError]`

Execute a SQL query and return result.

#### `conn.execute(sql: String, params: Array[Value]) -> Result[QueryResult, PgError]`

Execute a SQL query with parameters. Parameters are automatically substituted for placeholders like `$1`, `$2`, etc.

```moonbit
// Example: parameterized query with automatic substitution
let rows = conn.execute_and_fetch(
  "SELECT * FROM users WHERE id = $1 AND name = $2",
  [@postgres.Value::Int(123), postgres.Value::String("John")]
)
```

**Security Note**: `execute()` with parameters is more secure than manual string concatenation, but for production use, prefer `prepare()` + `execute_prepared()` for better performance and proper parameter binding.

#### `conn.query_and_fetch(sql: String) -> Result[Array[Array[String]], PgError]`

Execute a query and automatically fetch rows, then free the result.

```moonbit
let rows = conn.query_and_fetch("SELECT * FROM users")
```

#### `conn.execute_and_fetch(sql: String, params: Array[Value]) -> Result[Array[Array[String]], PgError]`

Execute a parameterized query, fetch rows, and automatically free the result.

#### `conn.prepare(name: String, sql: String) -> Result[PreparedStatement, PgError]`

Prepare a statement for repeated execution.

```moonbit
// Prepare a statement for repeated execution
let stmt = conn.prepare("get_user", "SELECT * FROM users WHERE id = $1")?
match stmt {
  Ok(prepared) => {
    // Execute with different parameters multiple times
    let rows1 = prepared.execute_and_fetch([@postgres.Value::Int(1)])
    let rows2 = prepared.execute_and_fetch([@postgres.Value::Int(2)])
    prepared.close()
  }
  Err(e) => { /* handle error */ }
}
```

### QueryResult Methods

#### `result.rows() -> Array[Array[String]]`

Get all rows from a query result as arrays of string values.

#### `result.columns() -> Array[String]`

Get column names from a query result.

#### `result.affected_rows() -> Int`

Get number of rows affected by INSERT/UPDATE/DELETE operations.

#### `result.free() -> Unit`

Free memory allocated for a query result.

### PreparedStatement Methods

#### `stmt.execute(params: Array[Value]) -> Result[QueryResult, PgError]`

Execute a prepared statement with parameters.

#### `stmt.execute_and_fetch(params: Array[Value]) -> Result[Array[Array[String]], PgError]`

Execute a prepared statement, fetch rows, and automatically free the result.

```moonbit
let rows = stmt.execute_and_fetch([@postgres.Value::Int(123)])
```

#### `stmt.close() -> Unit`

Close and free a prepared statement.

### `Value` Enum

Represents parameter values. MoonBit doesn't have an `any` type, so this enum serves a similar purpose, providing type-safe way to represent different PostgreSQL value types.

- `Null` - NULL value
- `Bool(Bool)` - Boolean value
- `Int(Int)` - Integer value
- `Int64(Int64)` - 64-bit integer value
- `Float(Float)` - Floating point value
- `String(String)` - String value
- `Bytes(Bytes)` - Binary data

**Type Safety**: Unlike dynamic `any` types, the `Value` enum ensures type safety through pattern matching. You must explicitly specify which type of value you're passing, which catches errors at compile time rather than runtime.

### Usage Examples

#### Simple Query

```moonbit
let conn = @postgres.connect("postgresql://user:pass@localhost/db")?
match conn.query_and_fetch("SELECT * FROM users") {
  Ok(rows) => {
    // Process rows...
  }
  Err(e) => {
    println("Query error: " + @postgres.PgError::to_string(e))
  }
}
```

#### Parameterized Query (Simple)

```moonbit
let conn = @postgres.connect("postgresql://user:pass@localhost/db")?
match conn.execute_and_fetch(
  "SELECT * FROM users WHERE id = $1",
  [@postgres.Value::Int(123)]
) {
  Ok(rows) => {
    // Process rows...
  }
  Err(e) => {
    println("Query error: " + @postgres.PgError::to_string(e))
  }
}
```

#### Prepared Statement (Recommended for Performance)

```moonbit
let conn = @postgres.connect("postgresql://user:pass@localhost/db")?
let stmt = conn.prepare("get_user", "SELECT * FROM users WHERE id = $1")?
match stmt {
  Ok(prepared) => {
    // Execute multiple times efficiently
    let rows1 = prepared.execute_and_fetch([@postgres.Value::Int(1)])
    let rows2 = prepared.execute_and_fetch([@postgres.Value::Int(2)])
    let rows3 = prepared.execute_and_fetch([@postgres.Value::Int(3)])
    prepared.close()
  }
  Err(e) => {
    println("Prepare error: " + @postgres.PgError::to_string(e))
  }
}
conn.close()
```

### `PgError`

Error types that can be returned from operations.

- `ConnectionError(String)` - Connection failed
- `AuthError(String)` - Authentication failed
- `QueryError(String)` - Query execution failed
- `ProtocolError(String)` - Protocol error

### Functions

#### `connect(config: ConnectConfig) -> Result[Connection, PgError]`

Establish a connection to PostgreSQL server.

#### `query(conn: Connection, sql: String) -> Result[QueryResult, PgError]`

Execute a SQL query and return result.

#### `close(conn: Connection) -> Unit`

Close the database connection.

#### `rows(result: QueryResult) -> Array[Array[String]]`

Get all rows from a query result as arrays of string values.

#### `columns(result: QueryResult) -> Array[String]`

Get column names from a query result.

#### `affected_rows(result: QueryResult) -> Int`

Get number of rows affected by INSERT/UPDATE/DELETE operations.

#### `free_result(result: QueryResult) -> Unit`

Free memory allocated for a query result.

## License

Apache-2.0
