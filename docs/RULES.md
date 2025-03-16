## Rules

An ALCA rule will typically include an **event type** that is being monitored, and a **condition** which makes use of
the received event's properties. Here's an example:

```
event file

rule detect_foo_ransomware : file {
    file.action == file.FILE_RENAME and file.new_name iendswith ".foo"
} 
```

Let's break it down:
1. The rule first checks if the file action matches the user-defined enum `FILE_RENAME` (meaning, a file was renamed).
   If this condition is true, then the second condition (on the right hand side of `and`) is evaluated.
2. The rule then does a **case-insensitive** check on the new file name to see if it ends with ".foo" (int other words, if
   the extension has changed). If both these conditions are true, then the rule is flagged as true and we receive a
   notification similar to the one below:

```
[2025-01-01 00:00:00] [rule] [malware.exe] name = "detect_foo_ransomware"
```

Rules do not need to include an event type, but this would mean that you won't be accessing event data in the rule, so
what's the point?

```
rule dummy_rule { 1 + 1 == 2 }
```

#### Private rules

Private rules are rules that you don't want a notification from if triggered. They will, however, be run on event data
that is received just like any other rule (so long as the received event's type matches the event that the rule is 
monitoring). This is so that [sequences](SEQUENCES.md) are able to utilise private rules.

To declare a private rule, simple use the `private` keyword before the rule declaration.

```
event file

private rule my_private_rule : file { file.name == "of_interest.txt" }
```

By including a rule identifier (public or private) in the body of another rule, ALCA effectively evaluates the 
referenced rule, and returns a boolean value that can be used in boolean operations. For example:

```
event file

private rule my_rule_a : file { file.name == "of_interest.txt" }

rule my_rule_b : file { my_rule_a and file.size_in_bytes > 10000 }
```

In this case, `my_rule_b` will trigger if a file's name is "of_interest.txt" AND its size is greater than 10,000 bytes.
> Note that a rule can only reference another rule if they are both monitoring the same event type.

### Types

ALCA can be described as statically typed. Event data field types in a module are explicitly declared,
and all types are checked for compatibility (with operators and each other) at compile time.

The 3 core types are booleans (true / false), integers, and strings. However, modules can also declare arrays
and structs containing these types, both of which ALCA has support for. 

Constant integer declarations in rule files can be either decimal (e.g. 10) or hexadecimal (e.g. 0xA).

ALCA rules contain a variety of operators that can be used when querying event data.

### Supported Operators

| Operators                                                                                           | Description            | Supported types | Resulting type |
|-----------------------------------------------------------------------------------------------------|------------------------|-----------------|----------------|
| `+`, `-`, `/` `*`                                                                                   | Arithmetic             | `int`           | `int`          |
| `==`, `!=`                                                                                          | Comparisons            | `int`, `string` | `boolean`      |
| `>=`, `>`, `<=`, `<`                                                                                | Integer comparisons    | `int`           | `boolean`      |
| `\|`, `^`, `&`, `~`, `<<`, `>>`                                                                     | Bitwise operators      | `int`           | `int`          |
| `#`                                                                                                 | String length operator | `string`        | `int`          |
| `and`, `or`, `not`, `!` (alias for `not`)                                                           | Logical operators      | `boolean`       | `boolean`      |
| `startswith`, `istartswith`, `endswith`, `iendswith`, `contains`, `icontains`, `iequals`, `matches` | String operators       | `string`        | `boolean`      |

### Bitwise Operators

| Operator | Description     | Example usage                          |
|----------|-----------------|----------------------------------------|
| `\|`     | Bitwise OR      | `0x10 \| 0x01`                         |
| `^`      | Bitwise XOR     | `0x10 ^ 0x01`                          |
| `&`      | Bitwise AND     | `file.PERMS & file.FILE_READONLY != 0` |
| `~`      | Bitwise NOT     | `~23`                                  |
| `<<`     | Bit-shift left  | `0x01 << 8`                            |
| `>>`     | Bit-shift right | `0x10 >> 8`                            |

### String Length Operator

The `#` operator is a simple unary operator that goes in front of a string. It gets the length of the string as an integer.
The result can be used for integer operations, comparisons and so on.

Example:

```
#file.name == 32 and file.path icontains "\\Windows\\temp\\BadHive"
```

### String operators

ALCA provides a wide range of string operators. The operators beginning with `i` represent the case-insensitive versions.

| Operator                    | Description                        | Example usage                             |
|-----------------------------|------------------------------------|-------------------------------------------|
| `startswith`, `istartswith` | Check string prefix                | `file.path istartswith \\Windows\\temp`   |
| `endswith`, `iendswith`     | Check string suffix                | `file.path endswith badhive.exe`          |
| `contains`, `icontains`     | Check for substring in string      | `"badhive" contains "adh"`                |
| `iequals`                   | Case insensitive string comparison | `"malicious.exe" iequals "MALICIOUS.exE"` |
| `matches`                   | Regular expression matching        | `"badhive.exe" matches /.*\.EXE/i`        |

#### Regular expressions

ALCA makes use of [PCRE2's](https://github.com/PCRE2Project/pcre2) powerful perl-compatible regex engine. This
provides rule writers with a rich set of modifiers and features. ALCA supports the following modifiers, with valid
combinations of each also supported:

- `i` - `CASELESS`
- `s` - `DOTALL`
- `m` - `MULTILINE`
- `n` - `NO_AUTO_CAPTURE`

You can learn more about them [here](https://www.pcre.org/current/doc/html/pcre2test.html#TOC1).

To define a regex in a rule, enclose the expression in forward-slashes `/`, and include the modifier(s) on the end
of the final `/`. Make sure to test these out at [Regex101](https://regex101.com/)!

### Iterators

ALCA has support for iterator expressions. These iterate through an array, doing a boolean check on each item.
If a specified number of conditions are met, then the expression evaluates to true. They behave like and resemble [YARA's 
iterators](https://yara.readthedocs.io/en/stable/writingrules.html#iterators) prior to v4.0.

For example, the following expression checks that for **all** sections of a file, the section name either begins with 
".vmp" + a decimal digit, or has the name ".text".

```
rule check_vmprotect : file {
   file.num_sections >= and
   for 3 i in (0..file.num_sections) : (
      file.sections[i].name matches /\.vmp[0-9]/ or
      file.sections[i].name == ".text"
   )
}
```

The iterator syntax is as follows:

```
for <quantifier> <variable> in (<range_start> .. <range_end>) : ( <boolean_expression> )
```

Valid quantifiers are any 32-bit integer, `any` and `all` keywords. When an integer N is specified, the iterator checks
that at least N conditions are satisfied. `any` checks for at least one condition being fulfilled, and `all` checks for
every condition in the range. They are effectively equivalent to:

- `any`: `for 1 i in (start..end)`
- `all`: `for end i in (start..end)`