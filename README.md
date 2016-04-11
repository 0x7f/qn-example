# Query Notifications Test

Based on the C-example from this easysoft blog post: http://www.easysoft.com/support/kb/kb01069.html

Make sure to check this blog post for further details:
https://dimarzionist.wordpress.com/2009/04/01/how-to-make-sql-server-notifications-work/
Especially about the requirements for valid sql select statements.

## Setup

```
USE master ALTER DATABASE VistraRBL SET ENABLE_BROKER;

USE VistraRBL
CREATE QUEUE SessionChangeMessages
CREATE SERVICE SessionChangeNotifications ON QUEUE SessionChangeMessages
([http://schemas.microsoft.com/SQL/Notifications/PostQueryNotification]);

USE VistraRBL
GRANT RECEIVE ON SessionChangeMessages TO "vistra"
GRANT SUBSCRIBE QUERY NOTIFICATIONS TO "vistra";
```

## Useful SQL queries

List current subscriptions:

```
SELECT * FROM sys.dm_qn_subscriptions;
```

Kill all running subscriptions:

```
KILL QUERY NOTIFICATION SUBSCRIPTION ALL;
```

Trigger change manually:

```
UPDATE dbo.Session SET sessionData=N'{"a":100}' WHERE sessionId='qeadssfd4q112asd';
```
