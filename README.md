# mattn/postgres

A PostgreSQL client library for MoonBit using libpq.

## Features

- Connection management with PostgreSQL servers
- Query execution
- Result parsing (rows, columns, affected rows)
- Error handling for connection and query failures

## Usage

```moonbit
use postgres

let config = postgres::ConnectConfig::{
  host: "localhost",
  port: 5432,
  database: "mydb",
  user: "postgres",
  password: "password"
}

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

### `ConnectConfig`

Configuration struct for PostgreSQL connection.

- `host: String` - Database host
- `port: Int` - Database port (default 5432)
- `database: String` - Database name
- `user: String` - Database user
- `password: String` - Database password

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
