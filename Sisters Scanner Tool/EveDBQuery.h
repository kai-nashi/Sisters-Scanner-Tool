# include <iostream>
# include <string>

# include <mysql.h>

class EveDBQuery
{
private:

        // this is string with part of query, public function insert arguments and return true query
    string  testQuery_0 = "SELECT id, english, russian FROM signaturesnames WHERE (ID = 1)";

    string signatureRead_0 = "SELECT signatures.id, signatures.systemID, signatures.code, signaturesnames.english as name, signatures.typeID",
           signatureRead_1 =	" FROM signatures JOIN signaturesnames",
           signatureRead_2 = " WHERE ( (signatures.systemID = ",
           signatureRead_3 = " AND signatures.code = '",
           signatureRead_4 = "') AND signatures.nameID = signaturesnames.id );";

    string signatureGetID_0 = "SELECT * FROM signatures WHERE (systemID = ",
           signatureGetID_1 = " AND code = '",
           signatureGetID_2 = "') ORDER BY created DESC LIMIT 1;";

    string  systemCheckID_0 = "SELECT id FROM systems WHERE (id = ",
            systemCheckID_1 = ") LIMIT 1;";

    string  systemCreate_0 = "SELECT systemCreate(",
            systemCreate_1 = ");";

public:

    const char* signatureCreate(int systemID, string sigCode, int typeID, string name, string nameByteCode, int languageID)
    // retrun query to get signature.id in system with code
    {
        if (languageID)
            name = nameByteCode;
        string query = "SELECT signatureCreate(" + to_string(systemID) + "," +
                                                   "'" + sigCode + "'" + "," +
                                                   to_string(typeID) + "," +
                                                   "'" + name + "'" + "," +
                                                   to_string(languageID) + ");";
        //cout << query << endl;
        return query.c_str();
    }

    const char* signatureGetID(int systemID, string sigCode)
    // return query to get signature.id in system with code
    {
        string query = signatureGetID_0 + to_string(systemID) + signatureGetID_1 + sigCode + signatureGetID_2;
        //cout << query << endl;
        return query.c_str();
    }

    const char* signatureRead(int systemID, string sigCode)
    // return query to get full information about signature
    {
        string query = signatureRead_0 + signatureRead_1 + signatureRead_2 +
                       to_string(systemID) + signatureRead_3 + sigCode + signatureRead_4;
        //cout << query << endl;
        return query.c_str();
    }

    const char* signatureUpdate(int sigID, int typeID, string name, string nameBytesStr, int languageID)
    // return query to update sigID
    {
        string query = "select signatureUpdate("+ to_string(sigID) + "," +
                       to_string(typeID) + ", '" + name + "', '"+ nameBytesStr +
                       "', " + to_string(languageID) + ")";
        //cout << query << endl;
        return query.c_str();
    }

    const char* systemCheckID(int systemID)
    // return query to get systemID by systemID
    {
        string query = systemCheckID_0 + to_string(systemID) + systemCheckID_1;
        //cout << query << endl;
        return query.c_str();
    }

    const char* systemCreate(int systemID, string systemName, float systemSecurity, int regionID, string regionName)
    // return query to get systemID by systemID
    {
        string query = systemCreate_0 + to_string(systemID) + ",'"
                                      + systemName + "',"
                                      + to_string(systemSecurity) + ","
                                      + to_string(regionID) + ",'"
                                      + regionName + "'"
                                      + systemCreate_1;
        //cout << query << endl;
        return query.c_str();
    }

    const char* testQuery()
    // return pointer ti test query str
    {
        return testQuery_0.c_str();
    }

protected:

};
