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
  ]
}
```

## Quick Start

```moonbit
fn main {
  let conn = mattn/postgres::connect("postgresql://user:password@localhost/dbname")?
  
  // Simple query
  let result = conn.query("SELECT * FROM users")?
  let rows = result.rows()
  result.free()
  
  // Parameterized query
  let result2 = conn.execute(
    "SELECT * FROM users WHERE id = $1",
    [mattn/postgres::from_int(42)]
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

### Value Creation

Use these functions to create parameter values:

```moonbit
@postgres::from_int(42)
@postgres::from_string("hello")
@postgres::from_bool(true)
```

For other value types, use direct construction where available:
- `Null`
- `Int64(123L)`
- `Float(1.5)`
- `Bytes(data)`

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
  let result = conn.execute(
    "SELECT * FROM users WHERE id = $1 AND active = $2",
    [mattn/postgres::from_int(42), mattn/postgres::from_bool(true)]
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
    let result = stmt.execute([mattn/postgres::from_int(i)])?
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
    [mattn/postgres::from_bool(false), mattn/postgres::from_int(123)]
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
