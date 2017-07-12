# include <iostream>
# include <string>

class EveLocation
{
private:

public:

    // eveHandle            - handle of eve online
    // nodeLocation         - node-list of location data
    // systemID             - id of current solar system
    // constellationID      - id of current constellation
    // regionID             - id of current region
    // address              - address of node-list of location data
    // languageID           - unicode code of language
    // systemName           - name of current solar system
    // constellationName    - name of current constellation
    // regionName           - name of current region

    HANDLE eveHandle = (HANDLE)0;
    EveNode nodeLocation;
    int systemID = 0, constellationID = 0, regionID = 1,
        address = 0;

    float systemSecurity = -1;
    string systemName, constellationName, regionName,
           path;

    void print()
    // print current location
    {
        if (systemID)
        {
                // print current location
            HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
            cout << systemName << " [";

                // set color to location security
            if (1 == systemSecurity)
                SetConsoleTextAttribute(hConsole, 11);
            else if (0.5 <= systemSecurity)
                SetConsoleTextAttribute(hConsole, 10);
            else if (0.0 < systemSecurity)
                SetConsoleTextAttribute(hConsole, 14);
            else
                SetConsoleTextAttribute(hConsole, 12);
            cout << setprecision(1) << fixed << systemSecurity;

                // and reset to white
            SetConsoleTextAttribute(hConsole, 15);
            cout << "] ";

            if (constellationID>1)
                cout << "< " << constellationName;

            if (regionID)
            {
                if (regionID == 1)
                    cout << " < " << "Wormhole" << endl;
                else
                    cout << " < " << regionName << endl;
            }

                // and print sysID, regID with gray color
            SetConsoleTextAttribute(hConsole, 8);
            cout << "System id [" << dec << systemID << "]; ";
            cout << "Region id [" << dec << regionID << "]; " << endl << endl;

                // reset to white
            SetConsoleTextAttribute(hConsole, 15);
        }
        else
            cout << "Can't reed current location" << endl;
    }

    void readLocation()
    // read unicode from address and pars it
    {

            // default all location info
        systemSecurity = -1;
        systemID = 0;
        constellationID = 0;
        regionID = 1;               // noregion on server
        systemName = "";
        constellationName = "";
        regionName = "";

            // try to read
        if (address)
        {
                // if nodeLocation was inited
            if (nodeLocation.init(eveHandle, address, "header", ""))
            {
                    // get header text
                int headerTextAddress = nodeLocation.propeties["_setText"];
                string headerText = readUnicode(eveHandle,headerTextAddress);

                    // get systemSecurity from text
                headerText = headerText.substr(0,headerText.find("</hint></color>"));
                headerText = headerText.substr(headerText.rfind(">")+1,headerText.size());
                systemSecurity = stof(headerText);

                    // get list of children for nodeLacation
                nodeLocation.readChildren();
                if (nodeLocation.children.find("_inlineObjects") == nodeLocation.children.end())
                    return;

                    // init header node and read children for it
                EveNode headerNode = nodeLocation.children["_inlineObjects"];
                headerNode.readChildren();

                    // for each children data in header node
                for(auto itter=headerNode.children.begin(); itter!=headerNode.children.end(); itter++)
                {
                    EveNode node = itter->second;

                        // if we can to know what we see
                    if (node.getPropetyAddress("urlID"))
                    {
                        node.readChildren();
                        string nameTemp, idTemp;

                            // try to get name address from extraAlt and textBuff
                        int nameTempAddress = node.readFromPath(node.name + "/extraAlt");
                        if (0 == nameTempAddress)
                            nameTempAddress = node.readFromPath(node.name + "/textBuff/1");

                            // if not found address to read
                        if (0 == nameTempAddress)
                            continue;

                            // read name
                        nameTemp = readUnicode(eveHandle, nameTempAddress);

                            // get what parameter has this name
                        if (node.getPropetyAddress("url"))
                        {
                            idTemp = readUnicode(eveHandle, node.propeties["url"]);
                            idTemp = idTemp.substr(idTemp.find("//")+2,idTemp.size());
                        }
                        else
                            continue;

                            // set data that match for urlID
                        switch (readPyInt(eveHandle, node.propeties["urlID"]))
                        {
                                // read system data
                            case 0:
                                {
                                    systemName = nameTemp;
                                    systemID = stoi(idTemp);
                                }
                            break;

                                // read constellation data
                            case 1:
                                {
                                    constellationName = nameTemp;
                                    constellationID = stoi(idTemp);
                                }
                            break;

                                // read region data
                            case 2:
                                {
                                    regionName = nameTemp;
                                    regionID = stoi(idTemp);
                                }
                            break;
                        }
                    }
                }
            }
        }
    }

protected:

};
