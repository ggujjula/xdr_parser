# xdr\_parser
xdr\_parser translates and XDR specification into Rust native data structures.

This project leverages (serde\_xdr)[https://github.com/jvff/serde-xdr] to provide serialization.

This project is a work in progress.

# Example
Running `./parser < test.x` with the following `test.x`:
```
const MAXUSERNAME = 32;     /* max length of a user name */
const MAXFILELEN = 65535;   /* max length of a file      */
const MAXNAMELEN = 255;     /* max length of a file name */

/*
 * Types of files:
 */
enum filekind {
   TEXT = 0,       /* ascii data */
   DATA = 1,       /* raw data   */
   EXEC = 0xff        /* executable */
};

/*
 * File information, per kind of file:
 */
union filetype switch (filekind kind) {
case TEXT:
   void;                           /* no extra information */
case DATA:
   string creator<MAXNAMELEN>;     /* data creator         */
case EXEC:
   string interpretor<MAXNAMELEN>; /* program interpretor  */
};

/*
 * A complete file:
 */
struct file {
   string filename<MAXNAMELEN>; /* name of file    */
   filetype type;               /* info about file */
   string owner<MAXUSERNAME>;   /* owner of file   */
   opaque data<MAXFILELEN>;     /* file data       */
};
```

Produces the following `generated.rs`:
```
/* Generated using xdr_parser */

extern crate serde_xdr;
extern crate serde_bytes;
#[macro_use]
extern crate serde_derive;
use std::io::Cursor;
use serde_bytes::ByteBuf;
use serde_xdr::{from_reader, to_writer};

macro_rules! MAXUSERNAME { () => { 32 }; }
macro_rules! MAXFILELEN { () => { 65535 }; }
macro_rules! MAXNAMELEN { () => { 255 }; }
#[derive(Serialize, Deserialize)]
enum filekind {
TEXT = 0x0,
DATA = 0x1,
EXEC = 0xff,
}
#[derive(Serialize, Deserialize)]
enum filetype {
filekind::TEXT{},
filekind::DATA{creator: String},
filekind::EXEC{interpretor: String},
}
#[derive(Serialize, Deserialize)]
struct file {
filename: String,
type: filetype,
owner: String,
data: ByteBuf,
}
```
