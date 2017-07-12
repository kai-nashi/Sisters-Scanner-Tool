# include <Windows.h>
# include <TlHelp32.h>

# include <iostream>
# include <string>
# include <vector>
# include <map>

using namespace std;

typedef int(* vFunction)(HANDLE arg1, int arg2);
typedef map<string, int> PyDict;

typedef struct tagENUMINFO
{
// In Parameters
   DWORD PId;

// Out Parameters
   HWND  hWnd;
   HWND  hEmptyWnd;
   HWND  hInvisibleWnd;
   HWND  hEmptyInvisibleWnd;
} ENUMINFO, *PENUMINFO;

BOOL CALLBACK EnumWindowsProc(HWND hWnd, LPARAM lParam)
{
   DWORD       pid = 0;
   PENUMINFO   pInfo = (PENUMINFO)lParam;
   TCHAR       szTitle[_MAX_PATH+1];

// sanity checks
   if (pInfo == NULL)
   // stop the enumeration if invalid parameter is given
      return(FALSE);

// get the processid for this window
   if (!::GetWindowThreadProcessId(hWnd, &pid))
   // this should never occur :-)
      return(TRUE);

// compare the process ID with the one given as search parameter
   if (pInfo->PId == pid)
   {
   // look for the visibility first
      if (::IsWindowVisible(hWnd))
      {
      // look for the title next
         if (::GetWindowText(hWnd, szTitle, _MAX_PATH) != 0)
         {
            pInfo->hWnd = hWnd;

         // we have found the right window
            return(FALSE);
         }
         else
            pInfo->hEmptyWnd = hWnd;
      }
      else
      {
      // look for the title next
         if (::GetWindowText(hWnd, szTitle, _MAX_PATH) != 0)
         {
            pInfo->hInvisibleWnd = hWnd;
         }
         else
            pInfo->hEmptyInvisibleWnd = hWnd;
      }
   }

// continue the enumeration
   return(TRUE);
}

HWND eveGetMainWindow(DWORD PId)
{
   ENUMINFO EnumInfo;

// set the search parameters
   EnumInfo.PId = PId;

// set the return parameters to default values
   EnumInfo.hWnd               = NULL;
   EnumInfo.hEmptyWnd          = NULL;
   EnumInfo.hInvisibleWnd      = NULL;
   EnumInfo.hEmptyInvisibleWnd = NULL;

// do the search among the top level windows
   ::EnumWindows((WNDENUMPROC)EnumWindowsProc, (LPARAM)&EnumInfo);

// return the one found if any
   if (EnumInfo.hWnd != NULL)
      return(EnumInfo.hWnd);
   else if (EnumInfo.hEmptyWnd != NULL)
      return(EnumInfo.hEmptyWnd);
   else if (EnumInfo.hInvisibleWnd != NULL)
      return(EnumInfo.hInvisibleWnd);
   else
      return(EnumInfo.hEmptyInvisibleWnd);
}

int eveGetProcessId()
{
    PROCESSENTRY32 entry;
    entry.dwSize = sizeof(PROCESSENTRY32);

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);

    if (Process32First(snapshot, &entry) == TRUE)
    {
        while (Process32Next(snapshot, &entry) == TRUE)
        {
            if (strcmp(entry.szExeFile, "exefile.exe") == 0)
            {
                return entry.th32ProcessID;
            }
        }
    }

    return 0;
}

HANDLE eveGetHandle(int evePID)
// return handle of eve online window
{
    HANDLE eveHandle = OpenProcess(PROCESS_VM_READ, FALSE, evePID);

    if (eveHandle != NULL)
        return eveHandle;

    return (HANDLE)0;
}

vector<int> findSubArray(BYTE* intArray, int intArraySize, BYTE* subArray, int subArraySize)
// return vector of index for subArray entry
{
    vector<int> pointers;

    for (int i=0; i<intArraySize-subArraySize+1; i++)
    {
        for (int j=0; j<subArraySize; j++)
        {
            if (intArray[i+j] != subArray[j]) break;
            if (j == subArraySize-1) pointers.push_back(i);
        }
    }

    return pointers;
}

vector<int> findOnPage(HANDLE pHandle, int address, BYTE* data, int len)
// return vector of index for data on page
{
    // read from memory
    // 4096 max size of memory page
    BYTE value[4096];
    vector<int> subIndex;

        // if read something
    if (0 < ReadProcessMemory(pHandle,(LPCVOID)address, value, 4096, NULL))
    {
            // try to find subArray
        subIndex = findSubArray(value, 4096, data, len);
        return subIndex;
    }
    else
    {
        return subIndex;
    }

}

unsigned int findPageNext(HANDLE pHandle, unsigned int addressStart, BYTE* data, int dataSize)
// return next address from addressStart of memory page with data
{
        // vars
    unsigned int addressPage = addressStart;
    vector<int> addressOnPage;

        // while page haven't data
    while (addressOnPage.size() == 0)
    {
            // check valid address
        if (addressPage >= 0x7FFFFFFF - 4096)
            return 0;

            // while page haven't data
        addressPage+=4096;
        addressOnPage = findOnPage(pHandle, addressPage, data, dataSize);
    }

        // if was found page
    if (addressOnPage.size() > 0)
        return addressPage;

        // if page didn't find
    return 0;
}

BYTE readByte(HANDLE pHandle, int address)
// return byte from address; 0 if error
{
    BYTE data[1];

    if (0 < ReadProcessMemory(pHandle,(LPCVOID)address, data, 1, NULL))
    {
        return data[0];
    }
    else return 0;
}

BYTE* readBytes(HANDLE pHandle, int address, int len)
// return bytes from address; 0 if error
{
    static BYTE* data;
    delete data;
    data = new BYTE[len];

    if (0 < ReadProcessMemory(pHandle,(LPCVOID)address, data, len, NULL))
    {
        return data;
    }

    return 0;
}

int readInt(HANDLE pHandle, int address)
// read PyInt value from address;
{
    int data[1];

    if (0 < ReadProcessMemory(pHandle,(LPCVOID)address, data, 4, NULL))
    {
        return data[0];
    }

    return 0;
}

double readDouble(HANDLE pHandle, int address)
// read PyFloat value from address;
{
    double data[1];

    if (0 < ReadProcessMemory(pHandle,(LPCVOID)address, data, 8, NULL))
    {
        return data[0];
    }

    return 0;
}

double readPyDouble(HANDLE pHandle, int address)
// read PyDouble value from address;
{
    return readDouble(pHandle, address+8);
}

int readPyInt(HANDLE pHandle, int address)
// read PyInt from address
{
    return readInt(pHandle, address+8);
}

string readType(HANDLE pHandle, int address)
// read PyType of data in address; return "" if error
{
        // read typeName
    string typeName;

        // get address of type name
    int type_address = readInt(pHandle, address+4);
    int type_address_str =  readInt(pHandle,type_address+12);
    int len = 0;

    if (address)
    {
            // while non zero char
        while (readByte(pHandle,type_address_str+len) > 0) len++;

            // when find len, +1 to end line
        if (len)
            len++;
        else
            return typeName;

        typeName = (char*)readBytes(pHandle, type_address_str, len+1);
    }

    return typeName;
}

string readString(HANDLE pHandle, int address)
// read PyString from address
{
    string stringData;

    int stringSize = readInt(pHandle, address+8);

    if (stringSize>0)
        stringData = (char*)readBytes(pHandle, address+20, stringSize+1);

    return stringData;
}

string readUnicode(HANDLE pHandle, int address)
// read unicode string from address
{
    string stringData;

        // if type of rreading data is unicode
    if ("unicode" == readType(pHandle, address))
    {
            // get size of string in unicode
        int stringSize = readInt(pHandle, address+8);
        int stringDataAddress = readInt(pHandle, address+12);

        if (stringSize)
        {
            BYTE * data = readBytes(pHandle, stringDataAddress, stringSize*2);

            for(int i=0; i<stringSize*2; i+=2)
                stringData.push_back(char(data[i]));
        }
    }

    return stringData;
}

PyDict readDict(HANDLE pHandle, int address)
// read PyDict from address
{
    PyDict data;

    if (address)
    {
        if ("dict" == readType(pHandle,address) || "Bunch" == readType(pHandle, address))
        {
            int dictSize = readInt(pHandle,address+8);

                    // dict data address
                address = readInt(pHandle,address+20);

                    // while count of elements in data < dictSize
                for (int i=0; data.size()<dictSize; i++)
                {
                        // dict data key
                    int dataKeyAddress = readInt(pHandle,address+12*i+4);

                    if (dataKeyAddress!=0)
                    {
                        int keylen = readInt(pHandle,dataKeyAddress+8);
                        if (keylen<50 and keylen>0)
                        {
                            string key=string((char*)readBytes(pHandle,dataKeyAddress+20,keylen+1));
                            int dataDataAddress = readInt(pHandle,address+12*i+8);
                            data[key] = dataDataAddress;
                        }
                    }
                }
        }
    }

    return data;
}

vector<int> readList(HANDLE pHandle, int address)
// return PyList as array of address PyList.datas
{
    vector<int> listData;

    if (address)
    {

        if( "list" == readType(pHandle,address))
        {
            int listSize = readInt(pHandle, address+8);
            int listAddress = readInt(pHandle, address+12);

            if (listAddress)
            {
                for (int i=0; i<listSize; i++)
                {
                    listData.push_back(readInt(pHandle,listAddress));
                    listAddress+=4;
                }
            }
        }
    }

    return listData;
}

bool isNode(HANDLE pHandle, int address)
// in address valid node
{
    int dictAddrs = readInt(pHandle, address+8);
    if ("dict" == readType(pHandle,dictAddrs))
        return TRUE;

    return FALSE;
}

string readNodeName(HANDLE pHandle, int address)
// return node name in address without initiate new node
{
    string nodeName;

    if (isNode(pHandle, address))
    {
        int dictAddrs = readInt(pHandle, address+8);
        PyDict dict = readDict(pHandle, dictAddrs);
        PyDict::iterator itter = dict.find("_name");

        if (itter != dict.end())
        {
            nodeName = readString(pHandle, itter->second);
        }
    }

    return nodeName;
}

int eveCheckAllFor(HANDLE pHandle, BYTE* data, int dataSize, char* type, int typePointer, vFunction funCheck)
// check all found address of data for type, and call funCheck if type correct
{
    int address = 0;
    vector<int> addressOnPage;

        // first page with
    unsigned int addressPage = findPageNext(pHandle, 0, data, dataSize);
        // while page address is valid
    while (addressPage > 0)
    {
            // get vector of index in page for data
        addressOnPage = findOnPage(pHandle, addressPage, data, dataSize);

            // for each index
        for (int i=0; i<addressOnPage.size(); i++)
        {
                // get address and type for data at address
            address = addressPage + addressOnPage.at(i);

                // if we can't get type
            if (readType(pHandle, address+typePointer).size() == 0)
                return 0;

                // check data type
            string typeName = readType(pHandle, address+typePointer);

                // if type is needed, start check it
            if (typeName == type)
            {
                address = funCheck(pHandle, address);

                    // if funCheck return non zero value
                    // that should be UIRoot
                if (address)
                    return address;
            }
        }

            // clear vector and find next page with data
        addressOnPage.clear();
        addressPage = findPageNext(pHandle, addressPage, data, dataSize);
    }

    return 0;
}

int eveCheckUIRoot(HANDLE pHandle, int address)
// check, that UIRoot in address is valid
{
        // get address and type for data at address
    if (address>0)
    {
        int dictAddress = readInt(pHandle, address+4);

        if (readType(pHandle, dictAddress).size() == 0)
            return 0;

        string typeName = readType(pHandle, dictAddress);
            // if type posible dict addres have dict
            // we found uiroot
        if (typeName == "dict")
        {
            PyDict uiDict = readDict(pHandle, dictAddress);

            if (uiDict.find("children") != uiDict.end() &&
                uiDict.find("renderSteps") != uiDict.end() &&
                uiDict.find("_sr") != uiDict.end())
            {
                if ("UIRoot" == readType(pHandle,address-4))
                    return address-4;
            }
        }
    }

    return 0;
}

int eveFindUIRoot(HANDLE pHandle, int address)
// call eveCheckAllFor for find UIRoot
{
        // convert int address to bytes
        // address -12 is address of PyStr
    int addressArray[] = {address-12};
    BYTE addressByte[4];
    memcpy(addressByte, addressArray, 4);

        // call cheker
    return eveCheckAllFor(pHandle, addressByte, 4, "UIRoot", -4, eveCheckUIRoot);
}

int eveFindStrObj(HANDLE pHandle, int address)
// call eveCheckAllFor for find PyString contains UIRoot
{
        // convert int address in data to bytes
        // +1 bcause [0 char1...charN 0]
        // -12 we found address of string
        // but we want address of PyStr
    int addressArray[] = {address+1};
    BYTE addressByte[4];
    memcpy(addressByte, addressArray, 4);

        // call cheker
    return eveCheckAllFor(pHandle, addressByte, 4, "type", -12, eveFindUIRoot);
}

int eveGetUIRoot(HANDLE pHandle)
// return address of UIRoot object
{
    BYTE data[8] = {0,0,0,0,0,0,0,0};
    BYTE byteCode[] = {"UIRoot"};

        // UIRoot in byteCode should start and end as zero byte
    for (int i=0; i<6; i++)
        data[i+1] = byteCode[i];

        // try to find UIRoot (-19 cause 0 char1 char2 char3 ... charN 0)
        // 1 byte for check, that befor string is empty byte
    return eveCheckAllFor(pHandle, data, 8, "str", -19, eveFindStrObj);
}
