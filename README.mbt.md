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
      "path": "mattn/postgres"
    }
  ],
  "link": {
    "native": {
      "cc-link-flags": "-lpq"
    }
  }
}
```

**Important**: The `link` section is required to link against libpq. For executable packages (like `cmd/main`), add this configuration to the package's `moon.pkg.json`.

## Quick Start

```moonbit
fn main {
  let conn = mattn/postgres::connect("postgresql://user:password@localhost/dbname")?
  
  // Simple query
  let result = conn.query("SELECT * FROM users")?
  let rows = result.rows()
  result.free()
  
  // Parameterized query with ToValue trait
  let result2 = conn.execute(
    "SELECT * FROM users WHERE id = $1",
    [42]
  )?
  result2.free()
  
  conn.close()
}
```

## API

### Connection

#### `connect(conninfo: String) -> Result[Connection, PgError]`

Connect to PostgreSQL. Format: `postgresql://user:password@host:port/database`

#### `Connection::query(sql: String) -> Result[QueryResult, PgError]`

Execute a query without parameters.

#### `Connection::execute[T : ToValue](sql: String, params: Array[T]) -> Result[QueryResult, PgError]`

Execute a query with parameters using `$1`, `$2`, etc. The `ToValue` trait allows you to pass values directly (e.g., `[42, "hello", true]`) or use explicit conversion functions.

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

#### `PreparedStatement::execute[T : ToValue](params: Array[T]) -> Result[QueryResult, PgError]`

Execute with parameters. The `ToValue` trait allows you to pass values directly.

#### `PreparedStatement::close() -> Unit`

Close the statement.

### Value Creation

The library provides a `ToValue` trait that allows you to pass values directly without wrapping them:

```moonbit
// Direct values - recommended
conn.execute("SELECT * FROM users WHERE id = $1", [42])?
conn.execute("SELECT * FROM users WHERE name = $1", ["Alice"])?
conn.execute("SELECT * FROM users WHERE active = $1", [true])?

// Mixed types
conn.execute(
  "INSERT INTO users (id, name, active) VALUES ($1, $2, $3)",
  [42, "Alice", true]
)?
```

Supported types that implement `ToValue`:
- `Int` → `Value::Int`
- `String` → `Value::String`
- `Bool` → `Value::Bool`
- `Int64` → `Value::Int64`
- `Float` → `Value::Float`
- `Bytes` → `Value::Bytes`
- `Value` → `Value` (passthrough)

For special cases, you can still use explicit `Value` constructors:
- `Value::Null` - for NULL values
- `Value::Int64(123L)` - explicit Int64
- `Value::Float(1.5)` - explicit Float
- `Value::Bytes(data)` - binary data

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
fn main {
  let conn = mattn/postgres::connect("postgresql://localhost/mydb")?
  let result = conn.query("SELECT id, name FROM users")?
  for row in result.rows() {
    println(row[0] + ": " + row[1])
  }
  result.free()
  conn.close()
}
```

### Parameterized Query

```moonbit
fn main {
  let conn = mattn/postgres::connect("postgresql://localhost/mydb")?
  // Direct value passing with ToValue trait
  let result = conn.execute(
    "SELECT * FROM users WHERE id = $1 AND active = $2",
    [42, true]
  )?
  result.free()
  conn.close()
}
```

### Prepared Statements

```moonbit
fn main {
  let conn = mattn/postgres::connect("postgresql://localhost/mydb")?
  let stmt = conn.prepare("get_user", "SELECT * FROM users WHERE id = $1")?
  for i = 1; i <= 100; i = i + 1 {
    let result = stmt.execute([i])?
    result.free()
  }
  stmt.close()
  conn.close()
}
```

### INSERT/UPDATE/DELETE

```moonbit
fn main {
  let conn = mattn/postgres::connect("postgresql://localhost/mydb")?
  let result = conn.execute(
    "UPDATE users SET active = $1 WHERE id = $2",
    [false, 123]
  )?
  println(result.affected_rows().to_string() + " rows updated")
  result.free()
  conn.close()
}
```

### Environment Variables

```moonbit
fn main {
  let host = mattn/postgres::pg_get_env("DB_HOST")
  let user = mattn/postgres::pg_get_env("DB_USER")
  let password = mattn/postgres::pg_get_env("DB_PASSWORD")
  let conninfo = "postgresql://" + user + ":" + password + "@" + host
  let conn = mattn/postgres::connect(conninfo)?
  conn.close()
}
```

## License

Apache-2.0
