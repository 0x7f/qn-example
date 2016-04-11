#include "qn.h"

int main() {
    SQLHANDLE envHandle;
    SQLHANDLE conHandle;
    connect_to_db(&envHandle, &conHandle);

    SQLCHAR szSql[] = "UPDATE VistraRBL.dbo.Session SET sessionData=N'{\"a\":23}' WHERE sessionId='qeadssfd4q112asd';";
    exec_query(conHandle, szSql);
    
    SQLDisconnect(conHandle);
    SQLFreeHandle(SQL_HANDLE_DBC, conHandle);
    SQLFreeHandle(SQL_HANDLE_ENV, envHandle);

    return EXIT_SUCCESS;
}