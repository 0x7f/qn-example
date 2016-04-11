#include "qn.h"

static void subscribe_to_query(SQLHANDLE conHandle, SQLCHAR* query) {
    SQLRETURN ret;
    SQLHANDLE hStmt;

    ret = SQLAllocHandle(SQL_HANDLE_STMT, conHandle, &hStmt);
    if (!SQL_SUCCEEDED(ret)) {
        extract_error("SQLAllocStmt hStmt", conHandle, SQL_HANDLE_DBC);
        exit(1);
    }

    ret = SQLSetStmtAttr(hStmt, SQL_SOPT_SS_QUERYNOTIFICATION_MSGTEXT, "Session has changed", SQL_NTS);
    if (!SQL_SUCCEEDED(ret)) {
        extract_error("SQLSetStmtAttr SQL_SOPT_SS_QUERYNOTIFICATION_MSGTEXT", hStmt, SQL_HANDLE_STMT);
        exit(1);
    }

    ret = SQLSetStmtAttr(hStmt, SQL_SOPT_SS_QUERYNOTIFICATION_OPTIONS, "service=SessionChangeNotifications", SQL_NTS);
    if (!SQL_SUCCEEDED(ret)) {
        extract_error("SQLSetStmtAttr SQL_SOPT_SS_QUERYNOTIFICATION_OPTIONS", hStmt, SQL_HANDLE_STMT);
        exit(1);
    }

    /* If you want to leave the query notification timeout set to its default (5 days), omit this line. */
    // TODO: re-subscribe before timeout occures
    // Notifications are sent only once. For continuous notification of data change, a new subscription
    // must be created by re-executing the query after each notification is processed.
    ret = SQLSetStmtAttr(hStmt, SQL_SOPT_SS_QUERYNOTIFICATION_TIMEOUT, "1", SQL_NTS);
    if (!SQL_SUCCEEDED(ret)) {
        extract_error("SQLSetStmtAttr SQL_SOPT_SS_QUERYNOTIFICATION_TIMEOUT", hStmt, SQL_HANDLE_STMT);
        exit(1);
    }

    /* We want to know if the data returned by running this query changes. */
    /* Not all queries are compatible with query notifications. Refer to */
    /* the SQL Server documentation for further information: */
    /* http://technet.microsoft.com/en-us/library/ms181122(v=sql.105).aspx */
    ret = SQLExecDirect(hStmt, query, SQL_NTS);
    if (!SQL_SUCCEEDED(ret)) {
        extract_error("SQLExecDirect", hStmt, SQL_HANDLE_STMT);
        exit(1);
    }

    SQLSMALLINT cols;
    ret = SQLNumResultCols(hStmt, &cols);
    printf("cols = %d %d\n", cols, ret);

    SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
}

static void wait_for_notification(SQLHANDLE conHandle) {
    SQLRETURN ret;
    SQLHANDLE hStmt;
    SQLSMALLINT cols;
    SQLCHAR buffer[4096];
    SQLLEN len;

    ret = SQLAllocHandle(SQL_HANDLE_STMT, conHandle, &hStmt);
    if (!SQL_SUCCEEDED(ret)) {
        extract_error("SQLAllocStmt hStmt", conHandle, SQL_HANDLE_DBC);
        exit(1);
    }
    
    /* This query retrieves the query notification message. */
    /* It will block until the timeout expires or the query's underlying */
    /* data changes. */
    ret = SQLExecDirect(hStmt, "WAITFOR (RECEIVE * FROM SessionChangeMessages)", SQL_NTS);
    if (!SQL_SUCCEEDED(ret)) {
        extract_error("WAITFOR", hStmt, SQL_HANDLE_STMT);
        exit(1);
    }

    print_query_result(hStmt);

    SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
}

int main() {
    SQLHANDLE envHandle;
    SQLHANDLE conHandle;
    connect_to_db(&envHandle, &conHandle);

    while (1) {
        printf(">>>>>>>>>>>>>>> SUBSCRIBING\n");
        subscribe_to_query(conHandle, szSubscribeQuery);
        wait_for_notification(conHandle);
    }
    
    SQLDisconnect(conHandle);
    SQLFreeHandle(SQL_HANDLE_DBC, conHandle);
    SQLFreeHandle(SQL_HANDLE_ENV, envHandle);

    return EXIT_SUCCESS;
}