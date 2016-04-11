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

    ret = SQLSetStmtAttr(hStmt, SQL_SOPT_SS_QUERYNOTIFICATION_TIMEOUT, (SQLPOINTER)SUBSCRIPTION_TIMEOUT, SQL_IS_UINTEGER);
    if (!SQL_SUCCEEDED(ret)) {
        extract_error("SQLSetStmtAttr SQL_SOPT_SS_QUERYNOTIFICATION_TIMEOUT", hStmt, SQL_HANDLE_STMT);
        exit(1);
    }

    ret = SQLExecDirect(hStmt, query, SQL_NTS);
    if (!SQL_SUCCEEDED(ret)) {
        extract_error("SQLExecDirect", hStmt, SQL_HANDLE_STMT);
        exit(1);
    }

    //print_query_result(hStmt);

    SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
}

static void wait_for_notification(SQLHANDLE conHandle) {
    SQLRETURN ret;
    SQLHANDLE hStmt;

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