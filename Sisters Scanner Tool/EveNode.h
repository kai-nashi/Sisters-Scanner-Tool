# include <Windows.h>

# include <iostream>
# include <string>
# include <vector>
# include <map>
# include <iomanip>

# include "EveMem.h"

using namespace std;

class EveNode
// Basic class of node
{
    private:

    public:

        int tempInt;

        // === PUBLIC VARS ===

        // eveHandle    - handle off eve online process
        // address      - address of node
        // path         - string path to node from UIRoot
        // name         - name of node
        // propeties    - map of propeties
        // children     - map of children nodes

        HANDLE eveHandle = (HANDLE)0;
        int address=0, addressPropeties=0;
        string path, defaultName, name;

        typedef map<string, int> nodePropeties;
        nodePropeties propeties;

        typedef map<string, EveNode> nodeChildren;
        nodeChildren children;

        // === PUBLIC FUNCTION ===

        int getPropetyAddress(string propetyName)
        // return propety address by name
        {
            if (propeties.find(propetyName) != propeties.end())
                return propeties[propetyName];

            return 0;
        }

        void printChildren()
        // print all children
        {
            if (children.size())
            {
                for (auto itter = children.begin(); itter != children.end(); itter++)
                    cout << left << setw(20) << name << itter->first << endl;
            }
            else
                cout << left << setw(20) << name << "No children." << endl;

        }

        void printPropeties()
        // print all children
        {
            if (propeties.size())
            {
                for (auto itter = propeties.begin(); itter != propeties.end(); itter++)
                    cout << left << setw(20) << name << itter->first << endl;
            }
            else
                cout << left << setw(20) << name << "No propeties." << endl;

        }

        int propetyGetValueInt(string propetyName)
        // return int of propety
        {
            if (propeties.find(propetyName) != propeties.end())
            {
                int address = propeties[propetyName];
                if (address)
                {
                    return readPyInt(eveHandle, address);
                }
            }
        }

        void propetySet(string propetyName, int propetyAddress)
        // add propety to propeties
        {
                // if is node, try to get real name for it
            if (isNode(eveHandle,propetyAddress))
            {
                string newName = readNodeName(eveHandle, propetyAddress);
                if (newName.size())
                    propetyName = newName;
            }

            propeties[propetyName] = propetyAddress;
        }

        void readChildren()
        // node create new map of children from propeties
        {
            children.clear();

            if (propeties.size())
            {
                    //parse propeties, find children and create for them nodes
                for(auto itter = propeties.begin(); (itter != propeties.end()); itter++)
                {
                    int childAddress = itter->second;

                    EveNode node;
                    if (node.init(eveHandle, childAddress, itter->first, path+name+"/"))
                        children[node.name] = node;
                }
            }
        }

        void readPropeties()
        // node create new map of propeties
        {
            propeties.clear();

            if (addressPropeties)
            {
                    //get dict of propeties
                PyDict dict = readDict(eveHandle, addressPropeties);

                    //parse dict and fill map propeties
                for(PyDict::const_iterator itter = dict.begin(); itter != dict.end(); itter++)
                {
                    propetySet(itter->first, itter->second);
                }
            }
        }

        void readPropetiesAddress()
        // node will get address of dictionary (DONT CALL FOR Bunch, dict, list)
        {
            if (address)
                addressPropeties = readPyInt(eveHandle,address);
        }

        void readPropetiesFromList(int addressList)
        // node will get propeties from list
        {
            propeties.clear();

            if (addressList)
            {
                    // get vector
                vector<int> listData = readList(eveHandle, addressList);

                    // if it has elements
                if (listData.size())
                {
                        // for each
                    for(int i=0; i<listData.size(); i++)
                        propetySet(to_string(i), listData.at(i));
                }
            }
        }

        void readName()
        // node will read name from self propeties
        {
                // if node has address
            if (propeties.size())
            {
                if (propeties.find("_name") != propeties.end())
                {
                        // read name
                    name = readString(eveHandle,propeties["_name"]);

                        // update path to each children
                    for( auto itter = children.begin(); itter != children.end(); itter++ )
                        (itter->second).path = path+name+"/";
                }
            }
        }

        int init(HANDLE pHandle, int nodeAddress, string nodeName, string nodePath)
        // create new instance with parameters, and return this
        {
                // simple data
            eveHandle = pHandle;
            name = nodeName;
            path = nodePath;

                // if address is valid
            if (nodeAddress)
            {

                    // get type of data from address
                string type = readType(eveHandle, nodeAddress);

                    // dict or Bunch
                if ("dict" == type || "Bunch" == type )
                {
                    addressPropeties = nodeAddress;
                    readPropeties();
                    readName();
                    return 1;
                }

                    // list
                else if ("list" == type)
                {
                    readPropetiesFromList(nodeAddress);
                    readName();
                    return 2;
                }

                    // if hear, childtype is node or simple data (int, float etc)
                else
                {
                        // check isNode, if it is, then normal initiate
                    if (isNode(eveHandle, nodeAddress))
                    {
                        address = nodeAddress;

                        readPropetiesAddress();
                        readPropeties();
                        readName();
                        return 3;
                    }
                }
            }

                // if node can't be init
            return 0;
        }

        int readFromPath(string pathToRead)
        // try to read address to value from path
        {
                // get first node in path
            int delimiter = pathToRead.find("/");
            string nodeName = pathToRead.substr(0,delimiter),
                   nodeNextPathToRead = pathToRead.substr(delimiter+1,pathToRead.size()),
                   nodeNextName = nodeNextPathToRead.substr(0,nodeNextPathToRead.find("/"));

                // if i'm current node
            if (nodeName == name)
            {
                    // get map element with key "nodeNextName" to itter
                nodePropeties::iterator itter = propeties.find(nodeNextName);

                    // if can find next path's point in propeties
                if (itter != propeties.end())
                {
                        // if next point is realy end of path
                    if (nodeNextName == nodeNextPathToRead)
                    {
                        return itter->second;
                    }
                    else
                    {
                            // if next point is not end of path
                            // try find children with nodeNextName

                            // create child node from address if it has been found in propeties
                        EveNode node;
                        if (node.init(eveHandle, itter->second, itter->first, path + name + "/"))
                        {
                            return node.readFromPath(nodeNextPathToRead);
                        }
                    }
                }
            }

                // error
            return 0;
        }
};

    // special subclass for UIroot
class EveNodeUIRoot: public EveNode
{
private:

public:

    int windowFind(string windowName)
    // return address for window with name windowName
    {

        int nodeAddressGameWindow = readFromPath("Desktop/children/_childrenObjects/l_main/children/_childrenObjects");

            // if address if node with all game windows has been found
        if (nodeAddressGameWindow)
        {
            int windowAddress;

                // create new node, that should have all open game window
                // node, that have all window is list. Shit happens :)
            EveNode nodeGameWindow;

            nodeGameWindow.init(eveHandle, 0, "_childrenObjects", "Desktop/children/_childrenObjects/l_main/children/");
            nodeGameWindow.readPropetiesFromList(nodeAddressGameWindow);
            nodeGameWindow.readName();
            nodeGameWindow.readChildren();

                // try to find window
            windowAddress = nodeGameWindow.readFromPath(nodeGameWindow.name + "/" + windowName);

                // if it find, return address
            if (windowAddress)
                return windowAddress;

                // if window not found. it may be in container with other window
            for(auto itter = nodeGameWindow.children.begin(); itter != nodeGameWindow.children.end(); itter++)
            {
                EveNode node = itter->second;

                    // try find it in every children
                windowAddress = node.readFromPath(node.name + "/_sr/__content/children/_childrenObjects/" + windowName);

                    // if it find, return address
                if (windowAddress)
                    return windowAddress;
            }

                // specifick path for some window

            // PROBE SCANNER
                // may be included in map window, try find it
            if ("probeScannerWindow" == windowName)
            {
                    // try find by specifick path
                windowAddress = readFromPath("Desktop/children/_childrenObjects/l_main/children/_childrenObjects/solar_system_map_panel/mapView/ProbeScannerPalette");

                   // if it find, return address
                if (windowAddress)
                    return windowAddress;
            }
        }

        return 0;
    }

    string windowFindPath(string windowName)
    // return path to windowName if it exist
    {
        string path;
        int nodeAddressGameWindow = readFromPath("Desktop/children/_childrenObjects/l_main/children/_childrenObjects");

            // if address if node with all game windows has been found
        if (nodeAddressGameWindow)
        {
            int windowAddress;

                // create new node, that should have all open game window
                // node, that have all window is list. Shit happens :)
            EveNode nodeGameWindow;

            nodeGameWindow.init(eveHandle, 0, "_childrenObjects", "Desktop/children/_childrenObjects/l_main/children/");
            nodeGameWindow.readPropetiesFromList(nodeAddressGameWindow);
            nodeGameWindow.readName();
            nodeGameWindow.readChildren();

                // try to find window
            windowAddress = nodeGameWindow.readFromPath(nodeGameWindow.name + "/" + windowName);

                // if it find, return address
            if (windowAddress)
            {
                path = nodeGameWindow.path + nodeGameWindow.name + "/" + windowName;
                return path;
            }

                // if window not found. it may be in container with other window
            for(auto itter = nodeGameWindow.children.begin(); itter != nodeGameWindow.children.end(); itter++)
            {
                EveNode node = itter->second;

                    // try find it in every children
                windowAddress = node.readFromPath(node.name + "/_sr/__content/children/_childrenObjects/" + windowName);

                    // if it find, return address
                if (windowAddress)
                {
                    path = node.path + node.name + "/_sr/__content/children/_childrenObjects/" + windowName;
                    return path;
                }
            }

                // specifick path for some window
            string specifickPath;

            // PROBE SCANNER
                // may be included in map window, try find it
            if ("probeScannerWindow" == windowName)
            {
                    // try find by specifick path
                specifickPath = "Desktop/children/_childrenObjects/l_main/children/_childrenObjects/solar_system_map_panel/mapView/ProbeScannerPalette";
                windowAddress = readFromPath(specifickPath);

                   // if it find, return address
                if (windowAddress)
                    return specifickPath;
            }

        }

        return path;
    }

    int checkState(string viewState)
    // return True if viewstate is True
    {

        string viewStatePath = "Desktop/children/_childrenObjects/l_viewstate/children/_childrenObjects/" + viewState;
        int viewStateAddress = readFromPath(viewStatePath);

        if (readFromPath(viewStatePath))
        {
            string viewStatePath = "Desktop/children/_childrenObjects/l_viewstate/children/_childrenObjects/" + viewState;

            EveNode node;
            if (node.init(eveHandle, viewStateAddress, viewState, viewStatePath))
            {
                if (node.propeties.find("isopen") != node.propeties.end())
                    return readPyInt(eveHandle, node.propeties["isopen"]);
            }
        }

        return 0;
    }
};
