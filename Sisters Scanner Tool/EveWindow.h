# include <Windows.h>
# include <math.h>

# include <iostream>
# include <string>
# include <vector>
# include <map>

# include "EveNode.h"
# include "EveLocation.h"
# include "EveDB.h"
# include "EveScanResult.h"

using namespace std;

    // information about cell (x,y,height,width)
    // y is not used
typedef struct EveScanResultTableElementStruct
{
    int x, y, height, width;
};

    // information about scanresult for overlay
    // without scoll shift
typedef struct EveScanResultStruct
{
        // text for name and group
    string name, group;

    int fromTop;
    double strength;

        // column name
    int nameX = 0, nameY = 0,
        nameHeight = 16, nameWidth = 80;

        // column group
    int groupX = 0, groupY = 0,
        groupHeight = 16, groupWidth = 80;

        // text color
    int textColor = RGB(255, 255, 255);
    float textAlpha = 1;

        // background color
    int bgColor = RGB(10, 10, 10);
    float bgAlpha = 1;
};

class EveProbeScannerWindow
{
private:

public:

    // eveHandle            - handle of eve online window
    // inflight             - flag of flight state
    // paused               - flag of paused in work
    // windowNode           - node of this window
    // rootNode             - node of UIRoot

    // windowPropetiesPath  - path to propety from window (window name is not include);\
                              it's need, cause name of win may be probeWindowPanel and probeWindowPalette

    // windowPropeties      - propeties of probe scanner window (path of propeties start by probeScannerWindow)
    // rootPropeties        - propeties of UIRoot

    int evePID = 0;
    HWND eveHWND = (HWND)0;
    HANDLE eveHandle = (HANDLE)0;

    EveDB database;
    EveLocation location;

    EveNode windowNode, scanResultNode;
    EveNodeUIRoot rootNode;

    int inflight = 0, paused = 0, showOnlySignatures = 0;

    vector<EveScanResultTableElementStruct> scanResultColumns;
    vector<EveScanResultStruct> scanResultOverlay;

        // dict of EveScanResult; key is address of result node
    typedef map<int, EveScanResult> scanResultsDict;
    scanResultsDict scanResults;

        // dict of path to propeties;
    typedef map<string, string> winPropetiesPath;
    winPropetiesPath windowPropetiesPath, rootPropetiesPath;

        // dict of address of propeties;
    typedef map<string, int> winPropeties;
    winPropeties windowPropeties, rootPropeties;

        // dict of values of propeties;
    vector<string> propetiesNamesToValues;
    winPropeties propetiesValues;

    void initPropeties()
    // fill propeties (window and root) by EveNodePropety from window and root paths
    // and create node for scanResultNode
    {

            // fill window propeties
        windowPropeties.clear();
        for (auto itter=windowPropetiesPath.begin(); itter!=windowPropetiesPath.end(); itter++)
        {
                // get name of prop and path to
            string name = itter->first,
                   path = itter->second;

                // read from path address to prop
            int address = windowNode.readFromPath(windowNode.name + path);

                // if path valid
            if (address)
            {
                if (address != windowPropeties[name])
                    windowPropeties[name] = address;
            }
        }

            // fill UIRoot propeties
        rootPropeties.clear();
        for (auto itter=rootPropetiesPath.begin(); itter!=rootPropetiesPath.end(); itter++)
        {
                // get name of prop and path to
            string name = itter->first,
                   path = itter->second;

                // read from path address to prop
            int address = rootNode.readFromPath(path);

                // if path valid
            if (address)
            {
                if (address != rootPropeties[name])
                    rootPropeties[name] = address;
            }
        }

            // fill values of some propeties
        propetiesValues.clear();
        for (int i=0; i<propetiesNamesToValues.size(); i++)
        {
            propetiesValues[propetiesNamesToValues.at(i)] = getPropetyValueInt(propetiesNamesToValues.at(i));
        }

            // node of scanResults
        if (windowPropeties.find("scanResultNode") != windowPropeties.end())
        {
            scanResultNode.eveHandle = eveHandle;
            scanResultNode.name = "scanResultNode";
            scanResultNode.path = windowPropetiesPath["scanResultNode"];

            scanResultNode.readPropetiesFromList(windowPropeties["scanResultNode"]);
            scanResultNode.readChildren();
        }

            // if node with columns info found
        if (windowPropeties.find("scanResultColumns") != windowPropeties.end())
        {
                //clear columns info
            scanResultColumns.clear();

                // init this node with children
            EveNode node;
            node.init(eveHandle, windowPropeties["scanResultColumns"], "headers", "");
            node.readChildren();

                // if children list is not empty
            if (node.children.size())
            {
                    //read info about all columns
                for(auto itter=node.children.begin(); itter!=node.children.end(); itter++)
                {
                    EveScanResultTableElementStruct columnInfo;

                    columnInfo.x = (itter->second).propetyGetValueInt("_displayX");
                    columnInfo.y = (itter->second).propetyGetValueInt("_displayY");
                    columnInfo.height = (itter->second).propetyGetValueInt("_displayHeight");
                    columnInfo.width = (itter->second).propetyGetValueInt("_displayWidth");

                    scanResultColumns.push_back(columnInfo);
                }

                    // special math for group column
                    // group_column.width =  (probeScanWindow.width - group_column.x) - last_column.width
                scanResultColumns.at(4).width = (getSavedPropetyValue("width") - scanResultColumns.at(4).x) - scanResultColumns.at(5).width;
            }
        }
    }

    void initPropetiesPaths()
    // fill propetiesPaths as default, and then fillPropetiesFromPaths
    {
            // window propeties
        windowPropetiesPath["show"] = "/_display";

        windowPropetiesPath["x"] = "/_displayX";
        windowPropetiesPath["y"] = "/_displayY";

        windowPropetiesPath["height"] = "/_height";
        windowPropetiesPath["width"] = "/_width";

        windowPropetiesPath["scannerTools"] = "/scannerTools";

        windowPropetiesPath["scanResultPosY"] = "/scannerTools/ScanResultsContainer/_displayY";
        windowPropetiesPath["scanResultHeight"] = "/scannerTools/ScanResultsContainer/_displayHeight";
        windowPropetiesPath["scanResultsHeaderHeight"] = "/scannerTools/ScanResultsHeader/_displayHeight";
        windowPropetiesPath["scanResultsHeaderPosY"] = "/scannerTools/ScanResultsHeader/_displayY";

        windowPropetiesPath["scanResultNode"] = "/scannerTools/resultScroll/_sr/nodes";
        windowPropetiesPath["scanResultTotalHeight"] = "/scannerTools/resultScroll/_totalHeight";
        windowPropetiesPath["scanResultScrollPos"] = "/scannerTools/resultScroll/_position";

        windowPropetiesPath["scanResultColumns"] = "/scannerTools/sortHeaders/headers";

            // root propeties
        rootPropetiesPath["location"] = "Desktop/children/_childrenObjects/l_viewstate/children/_childrenObjects/l_view_overlays/children/_childrenObjects/l_sidePanels/children/_childrenObjects/sidePanel/children/_childrenObjects/1/mainCont/children/_childrenObjects/0/header";

            // we want save values of
        propetiesNamesToValues.push_back("show");
        propetiesNamesToValues.push_back("x");
        propetiesNamesToValues.push_back("y");
        propetiesNamesToValues.push_back("height");
        propetiesNamesToValues.push_back("width");
        propetiesNamesToValues.push_back("scanResultsHeaderHeight");
        propetiesNamesToValues.push_back("scanResultPosY");
        propetiesNamesToValues.push_back("scanResultsHeaderPosY");
        propetiesNamesToValues.push_back("scanResultHeight");
        propetiesNamesToValues.push_back("scanResultScrollPos");
    }

    void locationUpdate()
    // update location and if t change, database actions
    {
            //if propeties exist
        if (rootPropeties.find("location") != rootPropeties.end())
        {
                // if address has been changed
                // often it changes when we move at other system
            if (location.address != rootPropeties["location"])
            {
                    // get new address and read location
                location.address = rootPropeties["location"];
                location.readLocation();

                /* DB LEARNING FUNCTION */

                    // if location read
                if (location.systemID)
                {
                        // if database doesn't have current system
                    if (0 == database.systemCheckID(location.systemID))
                    {
                            // create location (with region if need)
                        location.systemID = database.systemCreate(location.systemID,
                                                                  location.systemName,
                                                                  location.systemSecurity,
                                                                  location.regionID,
                                                                  location.regionName);
                    }
                }

                /* DB LEARNING END */

                // get signatures for this system
            }
        }
    }

    void printLocation()
    // print current location info
    {
            // if location found
        if (rootPropeties.find("location") != rootPropeties.end())
        {
            location.print();
        }
        else
            cout << "Can't found current location" << endl;
    }

    void printScanResult()
    // print scan result
    {
            // if probeScannerWindow is found
        if (windowPropeties.find("scanResultNode") != windowNode.propeties.end())
        {
                // if has any scanResults
            if (scanResults.size())
            {
                // print each scanResult
                for (auto itter=scanResults.begin(); itter!=scanResults.end(); itter++)
                    (itter->second).print(showOnlySignatures);
            }

                // if no any results
            else
                cout << "No any results" << endl;
        }

            // if probe scanner didn't find
        else
            cout << "Can't find scan results" << endl;
    }

    void print()
    // print data
    {

            // clar screen
        system("cls");

            // if state is flight
        if (inflight)
        {
            printLocation();
            printScanResult();
        }
        else
        {
            cout << "Wait flight state." << endl;
        }
    }

    int getPropetyAddress(string propetyName)
    // return propety address by name
    {
        if (windowPropeties.find(propetyName) != windowPropeties.end())
            return windowPropeties[propetyName];

        return 0;
    }

    int getPropetyValueInt(string propetyName)
    // return propety value if int
    {
        if (windowPropeties.find(propetyName) != windowPropeties.end())
        {
            int address = windowPropeties[propetyName];

                // if we can read this value as int
            if ("int" == readType(eveHandle, address) ||
                "bool" == readType(eveHandle, address))
            {
                return readPyInt(eveHandle, address);
            }
        }

        return 0;
    }

    int getSavedPropetyValue(string propetyName)
    // return propety value from saved dict
    {
        if (propetiesValues.find(propetyName) != propetiesValues.end())
        {
            return propetiesValues[propetyName];
        }

        return 0;
    }

    void update()
    // update window data. Address of window, then propeties
    {
            // clear struct of overlay
        scanResultOverlay.clear();

            // check flight state
        inflight = rootNode.checkState("l_inflight");

            // if in flight state
        if (inflight)
        {

                // get address of probeScannerWindow by current path
            int address = rootNode.readFromPath(windowNode.path);

                // if path is wrong, find new
            if (address == 0)
            {
                    // try to find probeScannerWindow
                int winAddressProbeScanner = rootNode.windowFind("probeScannerWindow");

                    // if win can't be find, set address and addressProp for window node as zero
                    // now node can't read props and children, and readProp and readChildren just clear lists;
                if (winAddressProbeScanner == 0)
                {
                    windowNode.address = 0;
                    windowNode.addressPropeties = 0;
                    windowNode.readPropeties();
                    windowNode.readChildren();

                    return;
                }

                    // set new path of window node
                windowNode.path = rootNode.windowFindPath("probeScannerWindow");
            }

                // update lists of propeties and children
            windowNode.init(eveHandle, address, "probeScannerWindow", windowNode.path);
            windowNode.readChildren();

                // update all window propeties and UIroot propeties
            initPropeties();

                // update location
            int oldSystemID = location.systemID;
            locationUpdate();

                // if location change, clear list of signatures
            if (oldSystemID != location.systemID)
                scanResults.clear();

                // update scanResults
            resultsUpdate();
        }
    }

    void resultsUpdate()
    // update results
    {

            // create empty clear scanResults
        scanResultsDict scanResultsTemp;

            // try to find node in scanResults map, and it doesn't exist, create it
        for (auto itter=scanResultNode.children.begin(); itter!=scanResultNode.children.end(); itter++)
        {
            EveNode child = itter->second;
            EveScanResult scanResult;

                // if we didn't find EveScanResults for current node from scanResultsNode
                // node is Bunch, so it hasn't address, only address of dictionary
            if (scanResults.find(child.addressPropeties) == scanResults.end())
            {
                    // init scanResult;
                scanResult.init(eveHandle, child, &database, &location);
            }
            else
                scanResult = scanResults[child.addressPropeties];

            if (scanResult.signature)
                scanResultToOverlay(scanResult);

            scanResultsTemp[child.addressPropeties] = scanResult;
        }

            // scanResults with out non exist node
        scanResults = scanResultsTemp;
    }

    void scanResultToOverlay(EveScanResult scanResult)
    // convert EveScanResult to eveScanResultStruct (used in overlay)
    {
            // if we read all columns info
        if (scanResultColumns.size() == 6 && scanResult.overayDraw)
        {
                // create struct for scanResult
            EveScanResultStruct scanResultStruct;

                // read strength and position from top
            scanResultStruct.fromTop = scanResult.fromTop;
            scanResultStruct.strength = scanResult.strength;

                // set name if it not "Cosmic Signature"
            if ("Cosmic Signature" != scanResult.databaseName && scanResult.databaseName.size())
                scanResultStruct.name = scanResult.databaseName;

                // set type if it not "Unknow"
            if ("Unknow" != scanResult.typeStr && scanResult.typeStr.size())
                scanResultStruct.group = scanResult.typeStr;

                // read height of cell (-2 for strength line under scan result)
            scanResultStruct.nameHeight = scanResult.height-2;
            scanResultStruct.groupHeight = scanResult.height-2;

                // read width of cell
            scanResultStruct.nameWidth = scanResultColumns.at(3).width;
            scanResultStruct.groupWidth = scanResultColumns.at(4).width;

                // read x of cell
            scanResultStruct.nameX = scanResultColumns.at(3).x;
            scanResultStruct.groupX = scanResultColumns.at(4).x;

                // calculate y of cell
            scanResultStruct.nameY = scanResult.fromTop;
            scanResultStruct.groupY = scanResult.fromTop;

                // add struct to overlay data
            scanResultOverlay.push_back(scanResultStruct);
        }
    }

    int init(int pid, HANDLE phandle, HWND hwnd, int UIRootAddress)
    // return 1 if initiated
    // return 0 if any error
    {
            // easy variables
        evePID = pid;
        eveHWND = hwnd;
        eveHandle = phandle;

            // set eveHandle to lacation object
        location.eveHandle = eveHandle;

        cout << "   | Connecting to GUI: ";
        if (rootNode.init(eveHandle, UIRootAddress, "UIRoot", ""))
            cout << "OK" << endl;
        else
        {
            cout << "ERROR" << endl;
            cout << "   | Can't connect to GUI" << endl;
            return 0;
        }

            // read children of uiroot
        rootNode.readChildren();

            // try connect to database
        cout << "   | Connecting to database: ";
        if (database.connect())
            cout << "OK" << endl;
        else
        {
            cout << "ERROR" << endl;
            cout << "   | Can't connect to database" << endl;
            return 0;
        }

            // in class function for full initiate
        initPropetiesPaths();
        update();

        return 1;
    }

protected:
};

