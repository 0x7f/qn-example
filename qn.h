#pragma once

#pragma comment(lib, "odbc32.lib")
#pragma comment(lib, "msodbcsql11.lib")

#include <windows.h>

#include <stdio.h>
#include <sql.h>
#include <sqlext.h>
#include <stdlib.h>
#include <string.h>
#include <msodbcsql.h>

static SQLCHAR szDSN[] = "MY_ODBC_DRIVER_NAME";
static SQLCHAR szUser[] = "username";
static SQLCHAR szPass[] = "password";
static SQLCHAR szSubscribeQuery[] = "SELECT sessionId, sessionData, lastTouchedUtc FROM dbo.Session WHERE sessionId IS NOT NULL";

static void extract_error(char *fn, SQLHANDLE handle, SQLSMALLINT type) {
    SQLSMALLINT	 i = 0;
    SQLINTEGER	 native;
    SQLCHAR	 state[7];
    SQLCHAR	 text[256];
    SQLSMALLINT	 len;
    SQLRETURN	 ret;

    fprintf(stderr, "\nThe driver reported the following diagnostics whilst running %s\n\n", fn);

    do
    {
        ret = SQLGetDiagRec(type, handle, ++i, state, &native, text, sizeof(text), &len);
        if (SQL_SUCCEEDED(ret))
            printf("%s:%d:%ld:%s\n", state, i, native, text);
    } while (ret == SQL_SUCCESS);

    printf("\nPress enter to exit.\n");
    getchar();
}

static void connect_to_db(SQLHANDLE *envHandle, SQLHANDLE *conHandle) {
    SQLRETURN ret;

    ret = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, envHandle);
    if (!SQL_SUCCEEDED(ret)) {
        fprintf(stderr, "SQLAllocHandle failed\n");
        exit(1);
    }

    ret = SQLSetEnvAttr(*envHandle, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, SQL_IS_UINTEGER);
    if (!SQL_SUCCEEDED(ret)) {
        fprintf(stderr, "SQLSetEnvAttr conn failed\n");
        exit(1);
    }

    ret = SQLAllocHandle(SQL_HANDLE_DBC, *envHandle, conHandle);
    if (!SQL_SUCCEEDED(ret)) {
        fprintf(stderr, "SQLAllocHandle conn failed\n");
        exit(1);
    }

    ret = SQLConnect(*conHandle, szDSN, SQL_NTS, szUser, SQL_NTS, szPass, SQL_NTS);
    if (!SQL_SUCCEEDED(ret)) {
        extract_error("SQLDriverConnect conHandle", *conHandle, SQL_HANDLE_DBC);
        exit(1);
    }
}

static void print_query_result(SQLHANDLE hStmt) {
    SQLRETURN ret;
    SQLSMALLINT cols;
    SQLCHAR buffer[4096];
    SQLLEN len;

    ret = SQLNumResultCols(hStmt, &cols);
    if (!SQL_SUCCEEDED(ret)) {
        extract_error("SQLNumResultCols", hStmt, SQL_HANDLE_STMT);
        exit(1);
    }

    printf("cols = %d %d\n", cols, ret);
    if (cols == 0) {
        return;
    }

    while (1)
    {
        ret = SQLFetch(hStmt);
        if (SQL_SUCCEEDED(ret)) {
            // no-op
        }
        else if (ret == SQL_NO_DATA) {
            break;
        }
        else if (ret == SQL_ERROR) {
            printf(">>> %d\n", ret);
            extract_error("SQLFetch", hStmt, SQL_HANDLE_STMT);
            exit(1);
        }
        else {
            printf(">>> %d\n", ret);
            extract_error("SQLFetch", hStmt, SQL_HANDLE_STMT);
            exit(1);
        }

        printf("=========================\n");
        for (int i = 0; i < cols; i++) {
            printf("> %d: ", i);
            ret = SQLGetData(hStmt, i + 1, SQL_C_CHAR, buffer, sizeof(buffer), &len);
            if (len == SQL_NULL_DATA) {
                printf("NULL");
            }
            else {
                // TODO: hack for RECEIVE stmt
                if (i == 13) {
                    unsigned short val;
                    char x[5];

                    memcpy(x, buffer, 4);
                    x[4] = '\0';

                    val = strtol(x, NULL, 16) & 0xFFFF;
                    if (val == 0xFFFE) {
                        for (int j = 4; j < len; j += 4) {
                            memcpy(x, buffer + j, 4);
                            x[4] = '\0';

                            val = strtol(x, NULL, 16) & 0xFFFF;

                            if (val & 0x00FF) {
                                printf(".");
                            }
                            else {
                                val >>= 8;
                                printf("%c", val);
                            }
                        }
                    }
                    else {
                        printf("%s", buffer);
                    }
                }
                else {
                    printf("%s", buffer);
                }
            }
            printf("\n");
        }
        printf("\n");
    }
}

static void exec_query(SQLHANDLE conHandle, SQLCHAR* szQuery) {
    SQLRETURN ret;
    SQLHANDLE hStmt;

    ret = SQLAllocHandle(SQL_HANDLE_STMT, conHandle, &hStmt);
    if (!SQL_SUCCEEDED(ret)) {
        extract_error("SQLAllocStmt hStmt", conHandle, SQL_HANDLE_DBC);
        exit(1);
    }

    ret = SQLExecDirect(hStmt, szQuery, SQL_NTS);
    if (!SQL_SUCCEEDED(ret)) {
        extract_error("SQLExecDirect", hStmt, SQL_HANDLE_STMT);
        exit(1);
    }

    print_query_result(hStmt);

    SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
}
