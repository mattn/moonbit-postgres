# mattn/postgres

A PostgreSQL client library for MoonBit using libpq.

## Features

- Connection management with PostgreSQL servers
- Simple and parameterized query execution
- Prepared statements for efficient repeated execution
- Result parsing (rows, columns, affected rows)
- Error handling with typed error enum
- Environment variable access

## Requirements

- PostgreSQL development libraries (libpq-dev)
- Ubuntu/Debian: `sudo apt-get install libpq-dev`
- macOS: `brew install postgresql`
- Fedora/RHEL: `sudo dnf install postgresql-devel`

## Installation

Add to your `moon.pkg.json`:

```json
{
  "import": [
    {
      "path": "mattn/postgres",
      "alias": "postgres"
    }
  ]
}
```

## Quick Start

```moonbit
use postgres

fn main {
  let conn = postgres::connect("postgresql://user:password@localhost/dbname")?
  
  // Simple query
  let result = conn.query("SELECT * FROM users")?
  let rows = result.rows()
  result.free()
  
  // Parameterized query
  let result = conn.execute(
    "SELECT * FROM users WHERE id = $1",
    [@postgres.Value::Int(123)]
  )?
  result.free()
  
  conn.close()
}
```

## API

### Connection

#### `connect(conninfo: String) -> Result[Connection, PgError]`

Connect to PostgreSQL. Format: `postgresql://user:password@host:port/database`

#### `Connection::query(sql: String) -> Result[QueryResult, PgError]`

Execute a query without parameters.

#### `Connection::execute(sql: String, params: Array[Value]) -> Result[QueryResult, PgError]`

Execute a query with parameters using `$1`, `$2`, etc.

#### `Connection::prepare(name: String, sql: String) -> Result[PreparedStatement, PgError]`

Prepare a statement for repeated execution.

#### `Connection::close() -> Unit`

Close the connection.

### QueryResult

#### `QueryResult::rows() -> Array[Array[String]]`

Get all rows as string arrays.

#### `QueryResult::columns() -> Array[String]`

Get column names.

#### `QueryResult::affected_rows() -> Int`

Get affected row count for INSERT/UPDATE/DELETE.

#### `QueryResult::free() -> Unit`

Free result memory.

### PreparedStatement

#### `PreparedStatement::execute(params: Array[Value]) -> Result[QueryResult, PgError]`

Execute with parameters.

#### `PreparedStatement::close() -> Unit`

Close the statement.

### Value Types

```moonbit
@postgres.Value::Null
@postgres.Value::Bool(true)
@postgres.Value::Int(123)
@postgres.Value::Int64(123L)
@postgres.Value::Float(1.5)
@postgres.Value::String("hello")
@postgres.Value::Bytes(bytes)
```

Helper functions:
- `from_int(Int) -> Value`
- `from_string(String) -> Value`
- `from_bool(Bool) -> Value`

### Error Handling

```moonbit
pub enum PgError {
  ConnectionError(String)
  AuthError(String)
  QueryError(String)
  ProtocolError(String)
}

error.to_string() -> String
```

### Utilities

#### `pg_get_env(name: String) -> String`

Get environment variable (returns empty string if not found).

## Examples

### SELECT

```moonbit
let conn = postgres::connect("postgresql://localhost/mydb")?
let result = conn.query("SELECT id, name FROM users")?
for row in result.rows() {
  println(row[0] + ": " + row[1])
}
result.free()
conn.close()
```

### Parameterized Query

```moonbit
let result = conn.execute(
  "SELECT * FROM users WHERE id = $1 AND active = $2",
  [@postgres.Value::Int(42), @postgres.Value::Bool(true)]
)?
result.free()
```

### Prepared Statements

```moonbit
let stmt = conn.prepare("get_user", "SELECT * FROM users WHERE id = $1")?
for i = 1; i <= 100; i = i + 1 {
  let result = stmt.execute([@postgres.Value::Int(i)])?
  // Process result...
  result.free()
}
stmt.close()
```

### INSERT/UPDATE/DELETE

```moonbit
let result = conn.execute(
  "UPDATE users SET active = $1 WHERE id = $2",
  [@postgres.Value::Bool(false), @postgres.Value::Int(123)]
)?
println(result.affected_rows().to_string() + " rows updated")
result.free()
```

### Environment Variables

```moonbit
let host = @postgres.pg_get_env("DB_HOST")
let user = @postgres.pg_get_env("DB_USER")
let password = @postgres.pg_get_env("DB_PASSWORD")
let conninfo = "postgresql://" + user + ":" + password + "@" + host
let conn = postgres::connect(conninfo)?
```

## License

Apache-2.0
