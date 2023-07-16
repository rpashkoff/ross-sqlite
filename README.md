# ROSS & SQLite model sample

## Intro

This repository contains ROSS (https://github.com/ROSS-org/ROSS) sample code which uses SQLite (https://www.sqlite.org/) database to read/store/manipulate model state data. This allows to simplify model state structure to several fields and have human-readable state snapshot at the right time.

SQLite can be configured to store data in several ways:
1. On system disk. This is a simplest, but the slowest way.
2. On RAM disk (example how to create https://www.jamescoyle.net/how-to/943-create-a-ram-disk-in-linux). 
  * Should be used carefully to avoid out of memory error.
  * The state snapshot is available after model process is completed.
3. On model process memory. 
  * Should be used carefully to avoid out of memory error. 
  * Provides the best performance.s
  * The model state will be lost when the process exists.

Executable should be started from the model folder ($ROSS_HOME/models/ross-sqlite) with command: ../../build/models/ross-sqlite/model_sqlite
## TODO

Add more comments