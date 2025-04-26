# Common Issues

## ``` #include "esp_bt.h" ``` is not found during build:
1. Delete `sdkconfig` file in project folder and 2.2.2.
2. Replace it with one located in `backup/sdkconfig`.
3. Do a full clean (or manually delete build folder).

## ESP32 memory issues:
1. Edit sdk config through vscode extension 
2. Open partition table section
3. Select "Custom Partition Table CSV" in first dropdown
4. In "Custom partition CSV file" tab - select `partitions.csv` as file name 

## Other esp_ packages not found (VScode Issue)

    If you are still expieriencing import issues it may be because you have not specified enviroment variables on your computer properly.

1. Navigate to .vscode\c_cpp_properties.json in project folder.

    Here are defined paths to compiler and tool folder where esp_ packages are located.

    Import error most likely is caused by incorrect path definitions

2. To fix this - open eviromental variables menu on your machine

    ⚠️ **Warning:** Thesee paths are **exemplatory** and depend on where your esp idf was installed!
    
    Add (on windows):
    - IDF_PATH: `C:\Users\{user_name}\esp\v5.3.2\esp-idf`
    - IDF_TOOLS_PATH: `C:\Users\{user_name}\esp`

    On linux (TODO: someone with linux pls complete):
    - TODO
    - TODO

3. Choose env variable 'Path' for user's env variables
4. Click Edit
5. Add `%IDF_PATH%\tools`
