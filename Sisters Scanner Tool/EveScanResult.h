# include <Windows.h>
# include <Vfw.h>

# include <iostream>
# include <string>
# include <map>

# include "md5.h"
//# include "EveScanResultStruct.h"

class EveScanResult
{

private:

public:

    // groupNameBytes           - database store bytecode for localisated names
    // signature                - flag, that scan result is cosmic signature (-1 - this result is not identify anytimes)
    // langugeID                - language of client (need for correct filling signaturesnames and signaturestypes)
    // typeID                   - index of scan result type (wormhole, combat site)
    // databaseSignatureID      - id in database (for this user)
    // databaseSignatureDataID  - id of signatureData that available for current user
    //                            if not exist - use public, if public not exist - create new (public or private)
    // distance                 - distance in meters
    // strength                 - strength of signal
    // code                     - code of signature in game
    // groupName                - string contains unicode data of signatures
    //                          it need for correct identification cosmic signatures
    // groupNameBytesStr        - string of groupNameBytes (server can read only bytecode as string for localisated name)
    // name                     - ingame signature name
    // nameBytesStr             - string of name in bytes (server can read only bytecode as string for localisated name)
    // databaseName             - name of signature in database
    // typeStr                  - string type of signature (wormhole, gas, relic)
    // distanceStr              - like ingame distance with m, au, km
    // strenghtStr              - float strength to string with two numbers after point and % at end
    // comments                 - database comments for this signatures (user comment is first, public next)
    // typeRes                  - contain path to icon of signature (use for identify signature type)
    // typeName                 - names of all types signatures (typeName[2] = wormhole)

    BYTE *groupNameBytes;
    int signature = 0, languageID = 0, typeID = 1,
        databaseSignatureID = 0, databaseTypeID = 0,
        height = 0, fromTop = 0, overayDraw = 1;
    double distance = 0, strength = 0;
    string code = "AAA-000", groupName, groupNameBytesStr,
           name, nameBytesStr, databaseName,
           typeStr, distanceStr, strenghtStr;

    vector<string> comments;

    typedef map<string, int> signatureTypeByResPath;
    signatureTypeByResPath typeRes;

    typedef map<int, string> signatureTypeIndex;
    signatureTypeIndex typeName;

    // EveLocation      - object of probeScannerWindow contain info about current location
    // database         - object of probeScannerWindow allow query to database
    // eveHandle        - handle of eve process
    // node             - node that represent scanResult info

    EveLocation *location;
    EveDB *database;
    HANDLE eveHandle;
    EveNode node;

    string bytesToString(BYTE *byteArray, int len)
    // return string of bytecode
    {
        string bytaArrayStr;

        for (int i=0; i<len; i++)
        {
            bytaArrayStr+=(to_string(byteArray[i]));
        }

        return bytaArrayStr;
    }

    string distanceToString()
    // convert double to Eve online distance
    {
        double distanceTemp;
        string units, distStr;

        if (distance<10000)
        {
            distanceTemp = round(distance*100)/100;
            units = "m";
        }
        else if (distance<10000000)
        {
            distanceTemp = round(distance*100)/100;
            units = "km";
        }
        else if (distance>=10000000)
        {
            distanceTemp=distance/149597870700;
            distanceTemp = round(distanceTemp*100)/100;
            units = "AU";
        }

        distStr = to_string(distanceTemp);
        distStr = distStr.substr(0,distStr.find(".")+3);
        return distStr + "[" + units + "]";
    }

    void databaseSignatureRead()
    // first initiate of signature
    {
            // get signature info
        vector<string> signatureInfo = database->databaseSignatureRead(location->systemID, code);

            // if we get info
        if (signatureInfo.size())
        {
            databaseSignatureID = stoi(signatureInfo.at(0));
            databaseName = signatureInfo.at(3);
            databaseTypeID = stoi(signatureInfo.at(4));
            typeStr = typeName[databaseTypeID];
        }

            // if signatureinfo didn't get
        if (databaseSignatureID == 0)
            databaseSignatureID = database->signatureCreate(location->systemID, code, typeID, name, md5(nameBytesStr), languageID);
    }

    void databaseSignatureUpdate()
    // update signature in database
    {
            // if we know signatureID
        if (databaseSignatureID)
        {
            int result = 0;

                // if we know signature name
            if (strength>=0.75)
                result = database->signatureUpdate(databaseSignatureID, typeID, name, md5(nameBytesStr), languageID);
            else
                result = database->signatureUpdate(databaseSignatureID, typeID, "", "", languageID);

                // and update database info
            if (result)
                databaseSignatureRead();
        }
    }

    void init(HANDLE pHandle, EveNode resultNode, EveDB *databaseObj, EveLocation *locationObj)
    // init vars
    {

        eveHandle = pHandle;
        node = resultNode;
        database = databaseObj;
        location = locationObj;

        typeRes["Unknow"] = 1;
        typeRes["res:/UI/Texture/Shared/Brackets/wormhole.png"] = 2;
        typeRes["res:/UI/Texture/Shared/Brackets/combatSite_16.png"] = 3;
        typeRes["res:/UI/Texture/Shared/Brackets/ore_site_16.png"] = 4;
        typeRes["res:/UI/Texture/Shared/Brackets/gas_site_16.png"] = 5;
        typeRes["res:/UI/Texture/Shared/Brackets/relic_site_16.png"] = 6;
        typeRes["res:/UI/Texture/Shared/Brackets/data_site_16.png"] = 7;

        typeName[1] = "Unknow";
        typeName[2] = "Wormhole";
        typeName[3] = "Combat site";
        typeName[4] = "Ore site";
        typeName[5] = "Gas site";
        typeName[6] = "Relic site";
        typeName[7] = "Data site";

            // read result
        readResult();

            // get i it signature
        signature = isSignature();

            // if localisated
        if (languageID)
            name = "";

            // if it signature
        if (signature)
        {
                // read from database
            databaseSignatureRead();

                // try update it
            databaseSignatureUpdate();
        }
    }

    int isSignature()
    // use only one time in first read result
    {
            // read address for scan groupName
        int scanGroupNameAddress = node.readFromPath(node.name + "/scanGroupName");

            // if it valid
        if (scanGroupNameAddress)
        {
                // read address for unicode string
            int groupNameBytesAddress = readInt(eveHandle, scanGroupNameAddress+12);

                // get address, get byte code of string
            if (groupNameBytesAddress)
            {
                int len = readPyInt(eveHandle, scanGroupNameAddress);
                groupNameBytes = readBytes(eveHandle, scanGroupNameAddress, len*2);
                groupNameBytesStr = bytesToString(groupNameBytes, len*2);
            }

                // read unicode
            string scanGroupName = readUnicode(eveHandle, scanGroupNameAddress);

                // if signature in english client
            if ("Cosmic Signature" == scanGroupName)
            {
                languageID = 0;
                return 1;
            }

                // if signature in russian client
            if ("AB>G=8:8 A83=0;>2" == scanGroupName)
            {
                languageID = 1;
                return 1;
            }

                // if signature in french client
            if ("Signature cosmique" == scanGroupName)
            {
                languageID = 2;
                return 1;
            }

                // if signature in german client
            if ("Kosmische Signatur" == scanGroupName)
            {
                languageID = 3;
                return 1;
            }

                // !!!special check for chinese!!!

                // chinese bytecode
            BYTE scanGroupNameChineseBytes[16] = {0x87, 0x5B, 0x99, 0x5B, 0x6E, 0x30, 0xB7, 0x30,
                                                  0xB0, 0x30, 0xCD, 0x30, 0xC1, 0x30, 0xE3, 0x30};

                // if address is valid
            if (groupNameBytesAddress)
            {
                    // read 16 bytes
                //groupNameBytes = readBytes(eveHandle, groupNameBytesAddress, 16);
                //groupNameBytesStr = bytesToString(groupNameBytes, 16);

                    // for each byte compair
                for(int i=0; i<16; i++)
                {
                        // if any byte is not equal, go out
                    if (groupNameBytes[i]!=scanGroupNameChineseBytes[i])
                        break;

                        // if it last byte that equals
                    if (i == 15)
                    {
                        languageID = 4;
                        return 1;
                    }
                }
            }
        }

        return 0;
    }

    int getType()
    // return type of scan result
    {
            // read path to icon for scan result
        string path = readString(eveHandle, node.readFromPath(node.name + "/sortValues/0"));

            // if can get type index by path
        if (typeRes.find(path) != typeRes.end())
        {
            return typeRes[path];
        }

            // return "Unknow" index
        return typeRes["Unknow"];
    }

    void print(int onlySignatures)
    // print this scan result
    {
            // if show only signatures then exit
        if (onlySignatures && signature <= 0)
            return;

            // print signature
            // if it signature, print [S], else [ ]
        if (signature)
            cout << left << setw(4) << "[S]";
        else
            cout << left << setw(4) << "[ ]";

        cout << left << setw(15) << distanceStr;
        cout << left << setw(10) << code;

            // if we have non default database name
        if (databaseName != "Cosmic Signature" && databaseName != "")
            cout << left << setw(25) << databaseName;
        else
            cout << left << setw(25) << typeStr;

        cout << left << setw(5) << strenghtStr << endl;

            // DEBUG INFO
        //cout << "Database ID: " << databaseSignatureID;
        //cout << "; Type: " << typeStr;
        //cout << "; Name: " << name << endl;
    }

    void readResult()
    // read result from node
    {
        node.readPropeties();
        if (node.propeties.size())
        {

                // get address for values
            int distanceAddress = node.readFromPath(node.name + "/distance"),
                strenghtAddress = node.readFromPath(node.name + "/sortValues/5"),
                codeAddress = node.readFromPath(node.name + "/id"),
                nameAddress = node.readFromPath(node.name + "/displayName"),
                heightAddrss = node.readFromPath(node.name + "/height"),
                fromTopAddress = node.readFromPath(node.name + "/positionFromTop");

                // read signature distance
            if (distanceAddress)
                distance = readPyDouble(eveHandle, distanceAddress);

                // read signature strength
            if (strenghtAddress)
                strength = readPyDouble(eveHandle, strenghtAddress);

            if (strength > 0.25)
                overayDraw = 0;

                // read signature code (id)
            if (codeAddress)
                code = readString(eveHandle, codeAddress);

                // read name and nameBytesStr
            if (nameAddress)
            {
                    // read name
                name = readUnicode(eveHandle, nameAddress);

                    // get len of unicode and address for unicode string data
                int len = readInt(eveHandle, nameAddress+8);
                int nameUnicodeDataAddress = readInt(eveHandle, nameAddress+12);

                    // if address and len is ok
                if (nameUnicodeDataAddress && len)
                {
                    BYTE* result;

                        // read byte for language
                    result = readBytes(eveHandle, nameUnicodeDataAddress, len*2);
                    nameBytesStr = bytesToString(result, len*2);

                        // if too long byte code
                    if (nameBytesStr.size()>100)
                        nameBytesStr = nameBytesStr.substr(0,100);
                }
            }

            if (heightAddrss)
                height = readPyInt(eveHandle, heightAddrss);

            if (fromTopAddress)
                fromTop = readPyInt(eveHandle, fromTopAddress);

                // update type and type string
            typeID = getType();
            typeStr = typeName[typeID];

                // update distance string
            distanceStr = distanceToString();

                // update strength string
            strenghtStr = to_string(round(strength*10000)/100);
            strenghtStr = strenghtStr.substr(0,strenghtStr.find(".")+3) + "%";
        }
    }

protected:

};
