// ===============================
// HYBRID INVENTORY SYSTEM
// PC + ESP32 Integration Project
// ===============================

#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <windows.h>
#include <cstdio>
#include <cstring>

using namespace std;

const string FILE_NAME = "inventory.csv";

// ---- DATABASE FUNCTIONS ----

void initializeDatabase() {
    FILE* fileCheck = fopen(FILE_NAME.c_str(), "r");

    if (fileCheck == NULL) {
        FILE* newFile = fopen(FILE_NAME.c_str(), "w");

        if (newFile != NULL) {
            fputs("ProductID,ProductName,Quantity,Price\n", newFile);
            fclose(newFile);

            cout << "[System] inventory.csv created with headers.\n";
        }
    }
    else {
        cout << "[System] Database linked successfully.\n";
        fclose(fileCheck);
    }
}

bool isUnique(string id) {
    FILE* file = fopen(FILE_NAME.c_str(), "r");

    if (file == NULL)
        return true;

    char buf[256];

    while (fgets(buf, sizeof(buf), file)) {

        buf[strcspn(buf, "\n")] = '\0';

        char bufCopy[256];
        strcpy(bufCopy, buf);

        char* currentID = strtok(bufCopy, ",");

        if (currentID != NULL && string(currentID) == id) {
            fclose(file);
            return false;
        }
    }

    fclose(file);
    return true;
}

void appendRecord(string data) {

    stringstream ss(data);

    string id;
    getline(ss, id, ',');

    if (isUnique(id)) {

        FILE* file = fopen(FILE_NAME.c_str(), "a");

        if (file != NULL) {

            fputs((data + "\n").c_str(), file);

            fclose(file);

            cout << "[Success] Record added.\n";
        }
    }
    else {
        cout << "[Error] ID already exists!\n";
    }
}

void searchByID(string targetID) {

    FILE* file = fopen(FILE_NAME.c_str(), "r");

    if (file == NULL) {
        cout << "[Error] Could not open database.\n";
        return;
    }

    char buf[256];
    bool found = false;

    while (fgets(buf, sizeof(buf), file)) {

        buf[strcspn(buf, "\n")] = '\0';

        string line(buf);

        if (line.substr(0, targetID.length()) == targetID) {

            cout << "\n>> MATCH FOUND: " << line << endl;

            found = true;
            break;
        }
    }

    if (!found)
        cout << "[Warning] ID " << targetID << " not found in database.\n";

    fclose(file);
}

void updateRecord(string id, string newData) {

    FILE* fileIn = fopen(FILE_NAME.c_str(), "r");
    FILE* fileOut = fopen("temp.csv", "w");

    if (fileIn == NULL || fileOut == NULL) {

        if (fileIn) fclose(fileIn);
        if (fileOut) fclose(fileOut);

        cout << "[Error] File error during update.\n";
        return;
    }

    char buf[256];
    bool updated = false;

    while (fgets(buf, sizeof(buf), fileIn)) {

        buf[strcspn(buf, "\n")] = '\0';

        char bufCopy[256];
        strcpy(bufCopy, buf);

        char* currentID = strtok(bufCopy, ",");

        if (currentID != NULL && string(currentID) == id) {

            fputs((newData + "\n").c_str(), fileOut);

            updated = true;
        }
        else {

            fputs((string(buf) + "\n").c_str(), fileOut);
        }
    }

    fclose(fileIn);
    fclose(fileOut);

    remove(FILE_NAME.c_str());

    rename("temp.csv", FILE_NAME.c_str());

    if (updated)
        cout << "[Success] Record updated.\n";
    else
        cout << "[Error] ID not found.\n";
}

void deleteRecord(string id) {

    FILE* fileIn = fopen(FILE_NAME.c_str(), "r");
    FILE* fileOut = fopen("temp.csv", "w");

    if (fileIn == NULL || fileOut == NULL) {

        if (fileIn) fclose(fileIn);
        if (fileOut) fclose(fileOut);

        cout << "[Error] File error during deletion.\n";
        return;
    }

    char buf[256];
    bool deleted = false;

    while (fgets(buf, sizeof(buf), fileIn)) {

        buf[strcspn(buf, "\n")] = '\0';

        char bufCopy[256];
        strcpy(bufCopy, buf);

        char* currentID = strtok(bufCopy, ",");

        if (currentID != NULL && string(currentID) == id) {

            deleted = true;
        }
        else {

            fputs((string(buf) + "\n").c_str(), fileOut);
        }
    }

    fclose(fileIn);
    fclose(fileOut);

    remove(FILE_NAME.c_str());

    rename("temp.csv", FILE_NAME.c_str());

    if (deleted)
        cout << "[Success] Record deleted successfully.\n";
    else
        cout << "[Error] ID not found.\n";
}

// ---- CORE HARDWARE INTEGRATION ----

string requestKeyFromESP32(string portName) {

    // 1. Open COM Port

    HANDLE hSerial = CreateFileA(
        portName.c_str(),
        GENERIC_READ | GENERIC_WRITE,
        0,
        0,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        0
    );

    if (hSerial == INVALID_HANDLE_VALUE)
        return "PORT_ERROR";

    // 2. Configure Serial Parameters

    DCB dcbSerialParams = {0};

    dcbSerialParams.DCBlength = sizeof(dcbSerialParams);

    GetCommState(hSerial, &dcbSerialParams);

    dcbSerialParams.BaudRate = CBR_115200;
    dcbSerialParams.ByteSize = 8;
    dcbSerialParams.StopBits = ONESTOPBIT;
    dcbSerialParams.Parity = NOPARITY;

    SetCommState(hSerial, &dcbSerialParams);

    // 3. Timeouts

    COMMTIMEOUTS timeouts = {0};

    timeouts.ReadIntervalTimeout = 50;
    timeouts.ReadTotalTimeoutConstant = 500;
    timeouts.ReadTotalTimeoutMultiplier = 10;

    timeouts.WriteTotalTimeoutConstant = 50;
    timeouts.WriteTotalTimeoutMultiplier = 10;

    SetCommTimeouts(hSerial, &timeouts);

    // 4. Wait for ESP32

    cout << "[System] Connection established. Waiting 2s for ESP32 to boot...\n";

    Sleep(2000);

    PurgeComm(hSerial, PURGE_RXCLEAR | PURGE_TXCLEAR);

    // 5. Send Command

    string cmd = "SEND_KEY\n";

    DWORD written;

    WriteFile(
        hSerial,
        cmd.c_str(),
        (DWORD)cmd.length(),
        &written,
        NULL
    );

    // 6. Read Response

    Sleep(200);

    char buf[64] = {0};

    DWORD read;

    if (ReadFile(hSerial, buf, sizeof(buf) - 1, &read, NULL) && read > 0) {

        string key = string(buf);

        key.erase(key.find_last_not_of(" \n\r\t") + 1);
        key.erase(0, key.find_first_not_of(" \n\r\t"));

        CloseHandle(hSerial);

        return key;
    }

    CloseHandle(hSerial);

    return "NO_RESPONSE";
}

// ---- MAIN INTERFACE ----

int main() {

    initializeDatabase();

    int choice;

    string port = "\\\\.\\COM9";

    while (true) {

        cout << "\n--- HYBRID INVENTORY SYSTEM ---" << endl;

        cout << "1. Hardware Search (Request Key from ESP32)" << endl;
        cout << "2. Manual Add Product" << endl;
        cout << "3. Update Product" << endl;
        cout << "4. Delete Product" << endl;
        cout << "5. Exit" << endl;

        cout << "Selection: ";

        cin >> choice;

        if (choice == 1) {

            cout << "[Action] Signaling ESP32..." << endl;

            string key = requestKeyFromESP32(port);

            if (key == "PORT_ERROR") {

                cout << "[Error] Cannot open " << port
                     << ". Close Serial Monitor or check connection!"
                     << endl;
            }

            else if (key == "NO_RESPONSE") {

                cout << "[Error] ESP32 did not respond. Check your wiring and code."
                     << endl;
            }

            else {

                cout << "[Hardware] ESP32 Sent Key: ["
                     << key << "]"
                     << endl;

                searchByID(key);
            }
        }

        else if (choice == 2) {

            string id, name, qty, price;

            cout << "ID: ";
            cin >> id;

            cout << "Name: ";
            cin >> name;

            cout << "Qty: ";
            cin >> qty;

            cout << "Price: ";
            cin >> price;

            appendRecord(id + "," + name + "," + qty + "," + price);
        }

        else if (choice == 3) {

            string id, name, qty, price;

            cout << "Target ID: ";
            cin >> id;

            cout << "New Name: ";
            cin >> name;

            cout << "New Qty: ";
            cin >> qty;

            cout << "New Price: ";
            cin >> price;

            updateRecord(
                id,
                id + "," + name + "," + qty + "," + price
            );
        }

        else if (choice == 4) {

            string id;

            cout << "Target ID to Delete: ";
            cin >> id;

            deleteRecord(id);
        }

        else if (choice == 5) {

            break;
        }
    }

    return 0;
}