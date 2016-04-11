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
    
    ret = SQLExecDirect(hStmt, "WAITFOR (RECEIVE * FROM SessionChangeMessages)", SQL_NTS);
    if (!SQL_SUCCEEDED(ret)) {
        extract_error("WAITFOR", hStmt, SQL_HANDLE_STMT);
        exit(1);
    }

    print_query_result(hStmt);

    SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
}

static int receive_queue_nowait(SQLHANDLE conHandle) {
    SQLRETURN ret;
    SQLHANDLE hStmt;

    ret = SQLAllocHandle(SQL_HANDLE_STMT, conHandle, &hStmt);
    if (!SQL_SUCCEEDED(ret)) {
        extract_error("SQLAllocStmt hStmt", conHandle, SQL_HANDLE_DBC);
        exit(1);
    }

    ret = SQLExecDirect(hStmt, "RECEIVE * FROM SessionChangeMessages", SQL_NTS);
    if (!SQL_SUCCEEDED(ret)) {
        extract_error("WAITFOR", hStmt, SQL_HANDLE_STMT);
        exit(1);
    }

    int numRows = 0;

    while (1) {
        ret = SQLFetch(hStmt);
        if (SQL_SUCCEEDED(ret)) {
            continue;
        }
        else if (ret == SQL_NO_DATA) {
            break;
        }
        else {
            extract_error("SQLFetch", hStmt, SQL_HANDLE_STMT);
            exit(1);
        }
    }

    SQLFreeHandle(SQL_HANDLE_STMT, hStmt);

    return numRows;
}

static void clear_query_notification_queue(SQLHANDLE conHandle) {
    while (receive_queue_nowait(conHandle) > 0) {
        // no-op
    }
}

int main() {
    SQLHANDLE envHandle;
    SQLHANDLE conHandle;
    connect_to_db(&envHandle, &conHandle);

    while (1) {
        clear_query_notification_queue(conHandle);
        subscribe_to_query(conHandle, szSubscribeQuery);
        wait_for_notification(conHandle);
    }
    
    SQLDisconnect(conHandle);
    SQLFreeHandle(SQL_HANDLE_DBC, conHandle);
    SQLFreeHandle(SQL_HANDLE_ENV, envHandle);

    return EXIT_SUCCESS;
}