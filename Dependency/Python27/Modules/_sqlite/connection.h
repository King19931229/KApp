/* connection.h - definitions for the connection type
 *
 * Copyright (C) 2004-2010 Gerhard H鋜ing <gh@ghaering.de>
 *
 * This file is part of pysqlite.
 *
 * This software is provided 'as-is', without any express or implied
 * warranty.  In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 */

#ifndef PYSQLITE_CONNECTION_H
#define PYSQLITE_CONNECTION_H
#include "Python.h"
#include "pythread.h"
#include "structmember.h"

#include "cache.h"
#include "module.h"

#include "sqlite3.h"

typedef struct
{
    PyObject_HEAD
    sqlite3* db;

    /* 1 if we are currently within a transaction, i. e. if a BEGIN has been
     * issued */
    int inTransaction;

    /* the type detection mode. Only 0, PARSE_DECLTYPES, PARSE_COLNAMES or a
     * bitwise combination thereof makes sense */
    int detect_types;

    /* the timeout value in seconds for database locks */
    double timeout;

    /* for internal use in the timeout handler: when did the timeout handler
     * first get called with count=0? */
    double timeout_started;

    /* None for autocommit, otherwise a PyString with the isolation level */
    PyObject* isolation_level;

    /* NULL for autocommit, otherwise a string with the BEGIN statement; will be
     * freed in connection destructor */
    char* begin_statement;

    /* 1 if a check should be performed for each API call if the connection is
     * used from the same thread it was created in */
    int check_same_thread;

    int initialized;

    /* thread identification of the thread the connection was created in */
    long thread_ident;

    pysqlite_Cache* statement_cache;

    /* Lists of weak references to statements and cursors used within this connection */
    PyObject* statements;
    PyObject* cursors;

    /* Counters for how many statements/cursors were created in the connection. May be
     * reset to 0 at certain intervals */
    int created_statements;
    int created_cursors;

    PyObject* row_factory;

    /* Determines how bytestrings from SQLite are converted to Python objects:
     * - PyUnicode_Type:        Python Unicode objects are constructed from UTF-8 bytestrings
     * - OptimizedUnicode:      Like before, but for ASCII data, only PyStrings are created.
     * - PyString_Type:         PyStrings are created as-is.
     * - Any custom callable:   Any object returned from the callable called with the bytestring
     *                          as single parameter.
     */
    PyObject* text_factory;

    /* remember references to functions/classes used in
     * create_function/create/aggregate, use these as dictionary keys, so we
     * can keep the total system refcount constant by clearing that dictionary
     * in connection_dealloc */
    PyObject* function_pinboard;

    /* a dictionary of registered collation name => collation callable mappings */
    PyObject* collations;

    /* if our connection was created from an APSW connection, we keep a
     * reference to the APSW connection around and get rid of it in our
     * destructor */
    PyObject* apsw_connection;

    /* Exception objects */
    PyObject* Warning;
    PyObject* Error;
    PyObject* InterfaceError;
    PyObject* DatabaseError;
    PyObject* DataError;
    PyObject* OperationalError;
    PyObject* IntegrityError;
    PyObject* InternalError;
    PyObject* ProgrammingError;
    PyObject* NotSupportedError;
} pysqlite_Connection;

extern PyTypeObject pysqlite_ConnectionType;

PyObject* pysqlite_connection_alloc(PyTypeObject* type, int aware);
void pysqlite_connection_dealloc(pysqlite_Connection* self);
PyObject* pysqlite_connection_cursor(pysqlite_Connection* self, PyObject* args, PyObject* kwargs);
PyObject* pysqlite_connection_close(pysqlite_Connection* self, PyObject* args);
PyObject* _pysqlite_connection_begin(pysqlite_Connection* self);
PyObject* pysqlite_connection_commit(pysqlite_Connection* self, PyObject* args);
PyObject* pysqlite_connection_rollback(pysqlite_Connection* self, PyObject* args);
PyObject* pysqlite_connection_new(PyTypeObject* type, PyObject* args, PyObject* kw);
int pysqlite_connection_init(pysqlite_Connection* self, PyObject* args, PyObject* kwargs);

int pysqlite_connection_register_cursor(pysqlite_Connection* connection, PyObject* cursor);
int pysqlite_check_thread(pysqlite_Connection* self);
int pysqlite_check_connection(pysqlite_Connection* con);

int pysqlite_connection_setup_types(void);

#endif
