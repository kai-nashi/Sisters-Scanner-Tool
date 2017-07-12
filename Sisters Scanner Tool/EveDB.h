# include <Windows.h>

# include <iostream>
# include <string>

# include <mysql.h>

# include "EveDBQuery.h"
# include "md5.h"

using namespace std;

class EveDB
{
private:

        // class construction of all mysql query
    EveDBQuery queries;

        // mysql conn object
    MYSQL *mysql_connection;

        // mysql connection propeties
    int port = 3306;
    //const char *host = "ricky.heliohost.org", *login = "evesst_user", *password = "A9oI]xuqN0yK", *db = "evesst_database";
    const char *host = "localhost", *login = "admin", *password = "admin", *db = "evesst_database";

    void printError()
    // print error with text of error
    {
        MessageBox(NULL,mysql_error(mysql_connection),"MySQL ERROR",MB_OK);
    }

public:

    int disconnect()
    // close connection
    {
        mysql_close(mysql_connection);
    }

    int connect()
    // connect to db
    {
            // descriptor of connect
        mysql_connection = mysql_init(NULL);
        if(mysql_connection == NULL)
        {
            printError();
            return 0;
        }

            // connection
        int error;
        mysql_real_connect(mysql_connection,
                           host, login, password, db, port, NULL,
                           CLIENT_MULTI_RESULTS);

            // if connection error
        if(mysql_errno(mysql_connection))
        {
            printError();
            mysql_close(mysql_connection);
            return 0;
        }

        return 1;
    }

    int signatureCreate(int systemID, string sigCode, int typeID, string name, string nameByteCode, int languageID)
    // return id for created signature
    {
        if (languageID)
            name = "";

        const char *query = queries.signatureCreate(systemID, sigCode, typeID, name, nameByteCode, languageID);
        return returnOneInt(query);
    }

    int signatureGetID(int systemID, string sigCode)
    // return signatureID
    {
        const char *query = queries.signatureGetID(systemID, sigCode);
        return returnOneInt(query);
    }

    vector<string> databaseSignatureRead(int systemID, string sigCode)
    // read signature information to scanResult
    {
        vector<string> signatureInfo;
        const char *query = queries.signatureRead(systemID, sigCode);
        int error;
        MYSQL_RES *qResult;
        MYSQL_ROW row;

            // send query
        error = mysql_query(mysql_connection, query);

            // if get error
        if (error)
        {
            printError();
            return signatureInfo;
        }

            // get results
        qResult = mysql_store_result(mysql_connection);

            // if have results
        if (NULL != qResult)
        {
                // get row from result and clear results
            row = mysql_fetch_row(qResult);
            mysql_free_result(qResult);

                // return result if it not null; 1 - no error, but return nosignature id
            if (row != NULL)
            {
                    // push all elements in result vector
                for (int i=0; i<5; i++)
                    signatureInfo.push_back(row[i]);
            }
        }

        return signatureInfo;
    }

    int signatureUpdate(int sigID, int typeID, string name, string nameBytesStr, int languageID)
    // return non zero if server tried update
    {
        const char *query = queries.signatureUpdate(sigID, typeID, name, nameBytesStr, languageID);
        return returnOneInt(query);
    }

    int systemCheckID(int systemID)
    // return systemID if system with id in database
    {
        const char *query = queries.systemCheckID(systemID);
        return returnOneInt(query);
    }

    int systemCreate(int systemID, string systemName, float systemSecurity, int regionID, string regionName)
    // create systemID and regionID (if need, and can create it)
    // return systemID if success
    {
        const char *query = queries.systemCreate(systemID, systemName, systemSecurity, regionID, regionName);
        return returnOneInt(query);
    }

    int returnOneInt(const char *query)
    // do query and return one iteger result from row[0]
    {
        int error;
        MYSQL_RES *qResult;
        MYSQL_ROW row;

            // send query
        error = mysql_query(mysql_connection, query);

            // if get error
        if (error)
        {
            printError();
            return 0;
        }

            // get results
        qResult = mysql_store_result(mysql_connection);

            // if have results
        if (qResult)
        {
                // get return result roe
                // and free results (create function return one row)
            row = mysql_fetch_row(qResult);
            mysql_free_result(qResult);

                // return result if it not null
            if (row != NULL)
                return stoi(row[0]);
            else
                return 0;
        }

            // error exit
        return 0;
    }

protected:

};
