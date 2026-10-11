test "string_to_c_bytes - ASCII is passed through and null-terminated" {
  let bytes = string_to_c_bytes("SELECT 1")
  // "SELECT 1" = 8 ASCII bytes + 1 null terminator
  assert_eq(bytes.length(), 9)
  assert_eq(bytes[0], b'S')
  assert_eq(bytes[7], b'1')
  assert_eq(bytes[8], (0 : Byte))
}

test "string_to_c_bytes - Japanese string encodes as UTF-8, not UCS-2" {
  // "り" is U+308A. UTF-8 = E3 82 8A. UCS-2 BE = 30 8A (bug).
  let bytes = string_to_c_bytes("り")
  assert_eq(bytes.length(), 4) // 3 UTF-8 bytes + null
  assert_eq(bytes[0], (0xE3 : Byte))
  assert_eq(bytes[1], (0x82 : Byte))
  assert_eq(bytes[2], (0x8A : Byte))
  assert_eq(bytes[3], (0 : Byte))
}

test "string_to_c_bytes - Korean Hangul encodes as 3-byte UTF-8 sequences" {
  // "김" is U+AE40. UTF-8 = EA B9 80.
  let bytes = string_to_c_bytes("김")
  assert_eq(bytes.length(), 4)
  assert_eq(bytes[0], (0xEA : Byte))
  assert_eq(bytes[1], (0xB9 : Byte))
  assert_eq(bytes[2], (0x80 : Byte))
  assert_eq(bytes[3], (0 : Byte))
}

test "string_to_c_bytes - Latin-1 supplement encodes as 2-byte UTF-8" {
  // "ç" is U+00E7. UTF-8 = C3 A7. Old UCS-2 path wrote bare E7 (invalid UTF-8).
  let bytes = string_to_c_bytes("ç")
  assert_eq(bytes.length(), 3)
  assert_eq(bytes[0], (0xC3 : Byte))
  assert_eq(bytes[1], (0xA7 : Byte))
  assert_eq(bytes[2], (0 : Byte))
}

test "string_to_c_bytes - empty string produces just the null terminator" {
  let bytes = string_to_c_bytes("")
  assert_eq(bytes.length(), 1)
  assert_eq(bytes[0], (0 : Byte))
}
