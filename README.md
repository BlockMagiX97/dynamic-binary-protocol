# DBPWV
Dynamic binary protocol without versioning
## Features
- Configurable
- Minimal
- Efficient
- Fast
## Goal
This project tries to make CONFIGURABLE binary network protocol, that doesn't need versioning (if you follow some rules).

## Requirements
- C compiler
## Install
## Usage
just define structs using macros like this.
In macro `STRUCTS`. first field is for macros. It cannot collide with other entries. second is identifier (string type). This is how the protocol identifies same fields, third is type name, fourth flag that indicates that struct is needed for the program. Now define a new macro with name `${first_field}_FIELDS`.
## Documentation
### Function documentation
### Struct documentation
