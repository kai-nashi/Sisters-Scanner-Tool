# include <iostream>
# include <string>

class EveScanResultStruct
{

private:

public:

        // text to draw
    string name, group;

        // flag of database data
    int database = 0, fromTop = 0;

        // name column propeties
    int nameX, nameY,
    nameHeight, nameWidth;

        // group column propeties
    int groupX, groupY,
    groupHeight, groupWidth;

        // text propeties
    int textColor = RGB(0, 0, 0);
    float textAlpha = 1;

        // bg propeties
    int bgColor = RGB(0, 0, 30);
    float bgAlpha = 1;

protected:

};
